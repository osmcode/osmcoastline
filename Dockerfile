FROM debian:jessie
RUN echo "deb http://ftp.fr.debian.org/debian jessie-backports main" > /etc/apt/sources.list.d/backports.list
RUN apt-get update
RUN apt-get install -y git build-essential cmake gdal-bin
RUN apt-get install -y libosmium2-dev libprotozero-dev libutfcpp-dev zlib1g-dev libgdal1-dev libgeos-dev sqlite3
COPY . /usr/lib/osmcoastline
RUN mkdir -p /usr/lib/osmcoastline/build
WORKDIR /usr/lib/osmcoastline/build
RUN cmake ..
RUN make
WORKDIR /usr/lib/osmcoastline

