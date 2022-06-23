FROM ubuntu:bionic
FROM gcc:latest

RUN apt-get update
RUN apt-get -y install vim nano

EXPOSE 8080

COPY ./server.c .

RUN gcc server.c -o server

CMD ["bin/bash"]
#CMD ["/bin/bash","-c","gdb udpserver"]