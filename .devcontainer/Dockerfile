FROM ubuntu:jammy

RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get install -y \
    build-essential \
    libsdl2-dev \
    cmake \
    git \
    && apt-get clean

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

# user name in container
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME -G root,dialout \
    && usermod -a -G root $USERNAME \
    && usermod -a -G plugdev $USERNAME
