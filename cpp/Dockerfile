FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt update -y
RUN apt install -y g++ make libssl-dev

RUN apt remove -y openmpi-bin libopenmpi-dev

RUN apt install -y mpich libmpich-dev

# RUN cd /tmp && wget https://github.com/pmodels/mpich/releases/download/v4.0.3/mpich-4.0.3.tar.gz && tar xfz mpich-4.0.3.tar.gz
# RUN cd /tmp/mpich-4.0.3 && ./configure --disable-fortran --disable-f77 --disable-fc --enable-fast --enable-cxx --enable-error-checking=no
# RUN cd /tmp/mpich-4.0.3 && make && make install

WORKDIR /code

COPY ./ /code/
# RUN useradd -ms /bin/bash dockeruser
# RUN chown -R  dockeruser ./
# USER dockeruser
RUN make clean && make

#RUN echo "alias mpirun='$(which mpirun) --allow-run-as-root'" > ~/.bashrc
CMD [ "bash" ]