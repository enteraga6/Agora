/**
 * @file radio_data_plane_soapy.cc
 * @brief Defination file for the RadioDataPlaneSoapy Class
 */

#include "radio_data_plane_soapy.h"

#include "SoapySDR/Time.hpp"
#include "logger.h"
#include "radio_soapysdr.h"

constexpr bool kDebugPrintRx = false;

RadioDataPlaneSoapy::RadioDataPlaneSoapy() : RadioDataPlane() {}

void RadioDataPlaneSoapy::Init(Radio* radio, const Config* cfg,
                               bool hw_framer) {
  return RadioDataPlane::Init(radio, cfg, hw_framer);
}

inline void RadioDataPlaneSoapy::Activate(Radio::ActivationTypes type) {
  return RadioDataPlane::Activate(type);
}

inline void RadioDataPlaneSoapy::Deactivate() {
  return RadioDataPlane::Deactivate();
}

inline void RadioDataPlaneSoapy::Close() { return RadioDataPlane::Close(); }

inline void RadioDataPlaneSoapy::Setup() {
  SoapySDR::Kwargs sargs;
  return RadioDataPlane::Setup(sargs);
}

int RadioDataPlaneSoapy::Rx(
    std::vector<std::vector<std::complex<int16_t>>>& rx_data, size_t rx_size,
    Radio::RxFlags& out_flags, long long& rx_time_ns) {
  std::vector<void*> rx_locations;
  rx_locations.reserve(rx_data.size());

  for (auto& buff : rx_data) {
    rx_locations.emplace_back(buff.data());
  }
  return RadioDataPlaneSoapy::Rx(rx_locations, rx_size, out_flags, rx_time_ns);
}

int RadioDataPlaneSoapy::Rx(
    std::vector<std::vector<std::complex<int16_t>>*>& rx_buffs, size_t rx_size,
    Radio::RxFlags& out_flags, long long& rx_time_ns) {
  std::vector<void*> rx_locations;
  rx_locations.reserve(rx_buffs.size());

  for (auto& buff : rx_buffs) {
    rx_locations.emplace_back(buff->data());
  }
  return RadioDataPlaneSoapy::Rx(rx_locations, rx_size, out_flags, rx_time_ns);
}

int RadioDataPlaneSoapy::Rx(std::vector<void*>& rx_locations, size_t rx_size,
                            Radio::RxFlags& out_flags, long long& rx_time_ns) {
  constexpr long kRxTimeout = 1;  // 1uS
  out_flags = Radio::RxFlags::None;
  //constexpr long kRxTimeout = 1000000;  // 1uS
  // SOAPY_SDR_ONE_PACKET; SOAPY_SDR_END_BURST
  int soapy_rx_flags = 0;

  int rx_status = 0;
  long long frame_time_ns(0);
  auto device = dynamic_cast<RadioSoapySdr*>(radio_)->SoapyDevice();

  rx_status = device->readStream(remote_stream_, rx_locations.data(), rx_size,
                                 soapy_rx_flags, frame_time_ns, kRxTimeout);

  if (rx_status > 0) {
    const size_t rx_samples = static_cast<size_t>(rx_status);

    //if end burst flag is not set, then we have partial data (hw_framer mode only)
    if (HwFramer()) {
      if (rx_samples != rx_size) {
        if ((soapy_rx_flags & SOAPY_SDR_END_BURST) == 0) {
          AGORA_LOG_TRACE(
              "RadioDataPlaneSoapy::Rx - short rx call %zu:%zu, more data "
              "could be available? %d\n",
              rx_samples, rx_size, soapy_rx_flags);
          //Soapy could print a 'D' if this happens. But this would be acceptable
        } else if ((soapy_rx_flags & SOAPY_SDR_END_BURST) ==
                   SOAPY_SDR_END_BURST) {
          AGORA_LOG_WARN(
              "RadioDataPlaneSoapy::Rx - short rx call %zu:%zu but it is the "
              "end of the rx samples %d\n",
              rx_samples, rx_size, soapy_rx_flags);
          out_flags = Radio::RxFlags::EndSamples;
        }
      } else {
        if ((soapy_rx_flags & SOAPY_SDR_END_BURST) == 0) {
          //This usually happens when the timeout is not long enough to wait for multiple packets for a given requested rx length
          AGORA_LOG_WARN(
              "RadioDataPlaneSoapy::Rx - expected SOAPY_SDR_END_BURST but "
              "didn't happen samples count %zu requested %zu symbols with "
              "flags %d\n",
              rx_samples, rx_size, soapy_rx_flags);
        }
      }

      if ((soapy_rx_flags & SOAPY_SDR_MORE_FRAGMENTS) ==
          SOAPY_SDR_MORE_FRAGMENTS) {
        AGORA_LOG_WARN(
            "RadioDataPlaneSoapy::Rx - fragments remaining on rx call for "
            "sample count %zu requested %zu symbols with flags %d\n",
            rx_samples, rx_size, soapy_rx_flags);
      }
      rx_time_ns = frame_time_ns;
    } else {
      // for UHD device (or software framer) recv using ticks
      rx_time_ns =
          SoapySDR::timeNsToTicks(frame_time_ns, Configuration()->Rate());
    }

    if (kDebugPrintRx) {
      std::printf(
          "Soapy RX return count %d out of requested %zu - flags: %d - HAS "
          "TIME: %d | END BURST: %d | MORE FRAGS: %d | SINGLE PKT: %d\n",
          rx_status, rx_size, soapy_rx_flags,
          (soapy_rx_flags & SOAPY_SDR_HAS_TIME) == SOAPY_SDR_HAS_TIME,
          (soapy_rx_flags & SOAPY_SDR_END_BURST) == SOAPY_SDR_END_BURST,
          (soapy_rx_flags & SOAPY_SDR_MORE_FRAGMENTS) ==
              SOAPY_SDR_MORE_FRAGMENTS,
          (soapy_rx_flags & SOAPY_SDR_ONE_PACKET) == SOAPY_SDR_ONE_PACKET);
    }

    if (kDebugRadioRX) {
      if (rx_status == static_cast<int>(Configuration()->SampsPerSymbol())) {
        std::cout << "Radio " << radio_->SerialNumber() << "(" << radio_->Id()
                  << ") received " << rx_status << " flags: " << out_flags
                  << " MTU " << device->getStreamMTU(remote_stream_)
                  << std::endl;
      } else {
        if (!((rx_status == SOAPY_SDR_TIMEOUT) && (out_flags == 0))) {
          std::cout << "Unexpected RadioRx return value " << rx_status
                    << " from radio " << radio_->SerialNumber() << "("
                    << radio_->Id() << ") flags: " << out_flags << std::endl;
        }
      }
    }
  } else if (rx_status == SOAPY_SDR_TIMEOUT) {
    /// If a timeout occurs tell the requester there are 0 bytes
    rx_status = 0;
  }
  return rx_status;
}

void RadioDataPlaneSoapy::Flush() {
  constexpr size_t kMaxChannels = 2;
  const long timeout_us(0);
  int flags = 0;
  long long frame_time(0);
  int r = 0;
  auto device = dynamic_cast<RadioSoapySdr*>(radio_)->SoapyDevice();

  std::vector<std::vector<std::complex<int16_t>>> samples(
      kMaxChannels,
      std::vector<std::complex<int16_t>>(Configuration()->SampsPerSymbol(),
                                         std::complex<int16_t>(0, 0)));
  std::vector<void*> ignore;
  ignore.reserve(samples.size());
  for (auto& ch_buff : samples) {
    ignore.emplace_back(ch_buff.data());
  }
  while (r > 0) {
    r = device->readStream(remote_stream_, ignore.data(),
                           Configuration()->SampsPerSymbol(), flags, frame_time,
                           timeout_us);
  }
}