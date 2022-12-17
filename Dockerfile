# This will eventually become a Dockerfile for Github Actions.
# https://docs.github.com/en/actions/creating-actions/creating-a-docker-container-action

FROM gcc
COPY RunCPM /usr/src/myapp/RunCPM
WORKDIR /usr/src/myapp/RunCPM
RUN make posix build
COPY . /usr/src/myapp/
RUN (pwd; mkdir -p A/0 B/0 ; cd A/0 ; unzip ../../../DISK/A)
CMD ["../docker-cmd.sh"]

