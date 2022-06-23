FROM ubuntu:bionic
FROM gcc:latest

RUN apt-get update

EXPOSE 8080

COPY ./client.c .

RUN gcc client.c -o client

#RUN [ "./client" "192.168.10.11" ]
CMD ["bin/bash"]
#CMD ["/bin/bash","-c","gdb udp_client"]