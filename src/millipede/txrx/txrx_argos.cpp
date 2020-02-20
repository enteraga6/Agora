/**
 * Author: Jian Ding
 * Email: jianding17@gmail.com
 * 
 */

#include "txrx.hpp"

PacketTXRX::PacketTXRX(Config* cfg, int RX_THREAD_NUM, int TX_THREAD_NUM, int in_core_offset)
{
    socket_ = new int[RX_THREAD_NUM];
    config_ = cfg;
    rx_thread_num_ = RX_THREAD_NUM;
    tx_thread_num_ = TX_THREAD_NUM;

    core_id_ = in_core_offset;
    tx_core_id_ = in_core_offset + RX_THREAD_NUM;

    /* initialize random seed: */
    srand(time(NULL));

    radioconfig_ = new RadioConfig(config_);
}

PacketTXRX::PacketTXRX(Config* cfg, int RX_THREAD_NUM, int TX_THREAD_NUM, int in_core_offset,
    moodycamel::ConcurrentQueue<Event_data>* in_queue_message, moodycamel::ConcurrentQueue<Event_data>* in_queue_task,
    moodycamel::ProducerToken** in_rx_ptoks, moodycamel::ProducerToken** in_tx_ptoks)
    : PacketTXRX(cfg, RX_THREAD_NUM, TX_THREAD_NUM, in_core_offset)
{
    message_queue_ = in_queue_message;
    task_queue_ = in_queue_task;
    rx_ptoks_ = in_rx_ptoks;
    tx_ptoks_ = in_tx_ptoks;
}

PacketTXRX::~PacketTXRX()
{
    delete[] socket_;
    radioconfig_->radioStop();
    delete radioconfig_;
    delete config_;
}

std::vector<pthread_t> PacketTXRX::startRecv(Table<char>& in_buffer, Table<int>& in_buffer_status, int in_buffer_frame_num, long long in_buffer_length, Table<double>& in_frame_start)
{
    buffer_ = &in_buffer; // for save data
    buffer_status_ = &in_buffer_status; // for save status
    frame_start_ = &in_frame_start;

    // check length
    buffer_frame_num_ = in_buffer_frame_num;
    // assert(in_buffer_length == packet_length * buffer_frame_num_); // should be integer
    buffer_length_ = in_buffer_length;
    printf("create RX threads\n");
    // new thread
    // pin_to_core_with_offset(RX, core_id_, 0);

    std::vector<pthread_t> created_threads;

    bool ret = radioconfig_->radioStart();
    if (!ret)
        return created_threads;

    for (int i = 0; i < rx_thread_num_; i++) {
        pthread_t recv_thread_;
        // record the thread id
        auto context = new EventHandlerContext<PacketTXRX>;
        context->obj_ptr = this;
        context->id = i;
        // start socket thread
        if (pthread_create(&recv_thread_, NULL, pthread_fun_wrapper<PacketTXRX, &PacketTXRX::loopRecv_Argos>, context) != 0) {
            perror("socket recv thread create failed");
            exit(0);
        }
        created_threads.push_back(recv_thread_);
    }
    sleep(1);
    pthread_cond_broadcast(&cond);
    //sleep(1);
    radioconfig_->go();
    return created_threads;
}

std::vector<pthread_t> PacketTXRX::startTX(char* in_buffer, int* in_buffer_status, int in_buffer_frame_num, int in_buffer_length)
{
    // check length
    tx_buffer_frame_num_ = in_buffer_frame_num;
    // assert(in_buffer_length == packet_length * buffer_frame_num_); // should be integer
    tx_buffer_length_ = in_buffer_length;
    tx_buffer_ = in_buffer; // for save data
    tx_buffer_status_ = in_buffer_status; // for save status
    // tx_data_buffer_ = in_data_buffer;
    printf("create TX or TXRX threads\n");
    // create new threads
    std::vector<pthread_t> created_threads;

    for (int i = 0; i < tx_thread_num_; i++) {
        pthread_t send_thread_;
        auto context = new EventHandlerContext<PacketTXRX>;
        context->obj_ptr = this;
        context->id = i;
        if (pthread_create(&send_thread_, NULL, pthread_fun_wrapper<PacketTXRX, &PacketTXRX::loopSend_Argos>, context) != 0) {
            perror("socket Transmit thread create failed");
            exit(0);
        }

        created_threads.push_back(send_thread_);
    }

    return created_threads;
}

void* PacketTXRX::loopRecv_Argos(int tid)
{
    //printf("Recv thread: thread %d start\n", tid);
    int radio_lo = tid * config_->nRadios / rx_thread_num_;
    int radio_hi = (tid + 1) * config_->nRadios / rx_thread_num_;
    int nradio_cur_thread = radio_hi - radio_lo;
    //printf("receiver thread %d has %d radios\n", tid, nradio_cur_thread);
    // get pointer of message queue
    pin_to_core_with_offset(Worker_RX, core_id_, tid);

    int packet_length = config_->packet_length;
    //// Use mutex to sychronize data receiving across threads
    pthread_mutex_lock(&mutex);
    printf("Thread %d: waiting for release\n", tid);

    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex); // unlocking for all other threads

    // use token to speed up
    //moodycamel::ProducerToken local_ptok(*message_queue_);
    moodycamel::ProducerToken* local_ptok = rx_ptoks_[tid];

    char* buffer = (*buffer_)[tid];
    int* buffer_status = (*buffer_status_)[tid];
    double* frame_start = (*frame_start_)[tid];

    // downlink socket buffer
    // char *tx_buffer_ptr = tx_buffer_;
    // char *tx_cur_buffer_ptr;
#if DEBUG_DOWNLINK
    size_t txSymsPerFrame = config_->dl_data_symbol_num_perframe;
    std::vector<size_t> txSymbols = config_->DLSymbols[0];
    std::vector<std::complex<int16_t>> zeros(config_->sampsPerSymbol);
#endif

    printf("receiver thread %d has %d radios\n", tid, nradio_cur_thread);

    // to handle second channel at each radio
    // this is assuming buffer_frame_num_ is at least 2
    int cursor = 0;
    long long frameTime;
    int prev_frame_id = -1;
    int nChannels = config_->nChannels;

    while (config_->running) {
        // receive data
        for (int rid = radio_lo; rid < radio_hi; rid++) { // FIXME: this must be threaded
            // if buffer is full, exit
            if (buffer_status[cursor] == 1) {
                printf("Receive thread %d buffer full, cursor: %d\n", tid, cursor);
                //for (int l = 0 ; l < buffer_frame_num_; l++)
                //    printf("%d ", buffer_status[l]);
                //printf("\n\n");
                config_->running = false;
                break;
            }

            struct Packet* pkt[nChannels];
            void* samp[nChannels];
            for (int ch = 0; ch < nChannels; ++ch) {
                pkt[ch] = (struct Packet*)&buffer[(cursor + ch) * packet_length];
                samp[ch] = pkt[ch]->data;
            }

            // this is probably a really bad implementation, and needs to be revamped
            while (config_->running && radioconfig_->radioRx(rid, samp, frameTime) <= 0)
                ;
            int frame_id = (int)(frameTime >> 32);
            int symbol_id = (int)((frameTime >> 16) & 0xFFFF);
            int ant_id = rid * nChannels;
#if MEASURE_TIME
            // read information from received packet
            // frame_id = *((int *)cur_ptr_buffer);
            if (frame_id > prev_frame_id) {
                *(frame_start + frame_id) = get_time();
                prev_frame_id = frame_id;
                if (frame_id % 512 == 200) {
                    _mm_prefetch((char*)(frame_start + frame_id + 512), _MM_HINT_T0);
                    // double temp = frame_start[frame_id+3];
                }
            }
#endif
            for (int ch = 0; ch < nChannels; ++ch) {
                new (pkt[ch]) Packet(frame_id, symbol_id, 0 /* cell_id */, ant_id + ch);
                // move ptr & set status to full
                buffer_status[cursor] = 1; // has data, after it is read it should be set to 0
                // push EVENT_RX_ENB event into the queue
                Event_data packet_message;
                packet_message.event_type = EVENT_PACKET_RECEIVED;
                // data records the position of this packet in the buffer &
                // tid of this socket (so that task thread could know which buffer it should visit)
                //packet_message.data = offset + tid * buffer_frame_num; // Note: offset < buffer_frame_num_
                packet_message.data = generateOffset2d_setbits(tid, cursor, 28);
                if (!message_queue_->enqueue(*local_ptok, packet_message)) {
                    printf("socket message enqueue failed\n");
                    exit(0);
                }
                ++cursor;
                cursor %= buffer_frame_num_;
            }
#if DEBUG_RECV
            printf("PacketTXRX %d: receive frame_id %d, symbol_id %d, ant_id %d, offset %d\n", tid, frame_id, symbol_id, ant_id, cursor);
#endif
#if DEBUG_DOWNLINK && !SEPARATE_TX_RX
            if (rx_symbol_id > 0)
                continue;
            for (size_t sym_id = 0; sym_id < txSymsPerFrame; sym_id++) {
                symbol_id = txSymbols[sym_id];
                int tx_frame_id = frame_id + TX_FRAME_DELTA;
                void* txbuf[2];
                long long frameTime = ((long long)tx_frame_id << 32) | (tx_symbol_id << 16);
                int flags = 1; // HAS_TIME
                if (symbol_id == (int)txSymbols.back())
                    flags = 2; // HAS_TIME & END_BURST, fixme
                if (ant_id != (int)config_->ref_ant)
                    txbuf[0] = zeros.data();
                else if (config_->getDownlinkPilotId(frame_id, symbol_id) >= 0)
                    txbuf[0] = config_->pilot_ci16.data();
                else
                    txbuf[0] = (void*)config_->dl_IQ_symbol[sym_id];
                radioconfig_->radioTx(ant_id / nChannels, txbuf, flags, frameTime);
            }
#endif
        }
    }
    return 0;
}

void* PacketTXRX::loopSend_Argos(int tid)
{
    pin_to_core_with_offset(Worker_TX, tx_core_id_, tid);

    int BS_ANT_NUM = config_->BS_ANT_NUM;
    int UE_NUM = config_->UE_NUM;
    //int OFDM_CA_NUM = config_->OFDM_CA_NUM;
    //int OFDM_DATA_NUM = config_->OFDM_DATA_NUM;
    //int subframe_num_perframe = config_->subframe_num_perframe;
    int data_subframe_num_perframe = config_->data_symbol_num_perframe;
    //int ul_data_subframe_num_perframe = config_->ul_data_subframe_num_perframe;
    //int dl_data_subframe_num_perframe = config_->dl_data_subframe_num_perframe;
    int packet_length = config_->packet_length;
    int nChannels = config_->nChannels;

    // downlink socket buffer
    // buffer_frame_num: subframe_num_perframe * BS_ANT_NUM * SOCKET_BUFFER_FRAME_NUM
    //int tx_buffer_frame_num = tx_buffer_frame_num_;
    //int* buffer_status = tx_buffer_status_;

    int ret;
    int offset;
    //int ant_id, symbol_id, frame_id;
    //struct timespec tv, tv2;

    //int txSymsPerFrame = 0;
    std::vector<size_t> txSymbols;
    if (config_->isUE) {
        //txSymsPerFrame = config_->ulSymsPerFrame;
        txSymbols = config_->ULSymbols[0];
    } else {
        //txSymsPerFrame = config_->dlSymsPerFrame;
        txSymbols = config_->DLSymbols[0];
    }
    std::vector<std::complex<int16_t>> zeros(config_->sampsPerSymbol);
    // use token to speed up
    // moodycamel::ProducerToken local_ptok(*message_queue_);
    //moodycamel::ConsumerToken local_ctok = (*task_queue_);
    // moodycamel::ProducerToken *local_ctok = (task_ptok[tid]);
    moodycamel::ProducerToken* local_ptok = rx_ptoks_[tid];
    while (config_->running) {

        Event_data task_event;
        //ret = task_queue_->try_dequeue(task_event);
        ret = task_queue_->try_dequeue_from_producer(*tx_ptoks_[tid], task_event);
        if (!ret)
            continue;

        // printf("tx queue length: %d\n", task_queue_->size_approx());
        if (task_event.event_type != TASK_SEND) {
            printf("Wrong event type!");
            exit(0);
        }

        //ant_id = task_event.data; //% config_->getNumAntennas();

        offset = task_event.data;
        int ant_id = offset % BS_ANT_NUM;
        int total_data_subframe_id = offset / BS_ANT_NUM;
        int frame_id = total_data_subframe_id / data_subframe_num_perframe;
        int current_data_subframe_id = total_data_subframe_id % data_subframe_num_perframe;

        int tx_subframe_id = current_data_subframe_id + UE_NUM;
        int socket_subframe_offset = offset % (SOCKET_BUFFER_FRAME_NUM * data_subframe_num_perframe * BS_ANT_NUM);
        struct Packet* pkt = (struct Packet*)&tx_buffer_[socket_subframe_offset * packet_length];
        char* tx_cur_buffer_ptr = (char*)pkt->data;
        frame_id += TX_FRAME_DELTA;

        //symbol_id = task_event.data / config_->getNumAntennas();
        //for (symbol_id = 0; symbol_id < txSymsPerFrame; symbol_id++)
        //{
        size_t symbol_id = tx_subframe_id; //txSymbols[tx_subframe_id];
        UNUSED void* txbuf[2];
        long long frameTime = ((long long)frame_id << 32) | (symbol_id << 16);
#if SEPARATE_TX_RX
        int flags = 1; // HAS_TIME
        if (symbol_id == txSymbols.back())
            flags = 2; // HAS_TIME & END_BURST, fixme
#endif
        int ch = ant_id % nChannels;
#if DEBUG_DOWNLINK
        if (ant_id != (int)config_->ref_ant)
            txbuf[ch] = zeros.data();
        else if (config_->getDownlinkPilotId(frame_id, symbol_id) >= 0)
            txbuf[ch] = config_->pilot_ci16.data();
        else
            txbuf[ch] = (void*)config_->dl_IQ_symbol[current_data_subframe_id];
#else
        txbuf[ch] = tx_cur_buffer_ptr + ch * packet_length;
#endif
            //buffer_status[offset+ch] = 0;
#if DEBUG_BS_SENDER
        printf("In TX thread %d: Transmitted frame %d, subframe %d, ant %d, offset: %d, msg_queue_length: %zu\n",
            tid, frame_id, symbol_id, ant_id, offset,
            message_queue_->size_approx());
#endif

        //clock_gettime(CLOCK_MONOTONIC, &tv);
#if SEPARATE_TX_RX
        radioconfig_->radioTx(ant_id / nChannels, txbuf, flags, frameTime);
#endif
        //clock_gettime(CLOCK_MONOTONIC, &tv2);

        //}

        Event_data tx_message;
        tx_message.event_type = EVENT_PACKET_SENT;
        tx_message.data = offset;
        if (!message_queue_->enqueue(*local_ptok, tx_message)) {
            printf("socket message enqueue failed\n");
            exit(0);
        }
    }
    return 0;
}