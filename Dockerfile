FROM debian:jessie

ADD . /scim

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install build-essential man bison pkg-config libzip-dev libxml2-dev libncursesw5-dev -y \
    && cd /scim/src \
    && make \
    && make install

CMD scim
