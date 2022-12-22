# This will eventually become a Dockerfile for Github Actions.
# https://docs.github.com/en/actions/creating-actions/creating-a-docker-container-action

# Invoke docker with something like
#
#     docker run -it .... "line 1" "line 2" "line 3"
#
# so that each command becomes a single argument.
#
# For consumer docker builds with data files, like assembler sources or others that can
# be processed non-interactively: "COPY . B/0" for B:, "COPY . C/0" for C:
# and "COPY . D/0" for D.
#
# File names MUST be uppercased.

FROM gcc

COPY RunCPM /usr/src/myapp/RunCPM
WORKDIR /usr/src/myapp/RunCPM
RUN make posix build

COPY . /usr/src/myapp/
RUN mkdir -p A/0 B/0 C/0 D/0; unzip -d A/0 -n ../DISK/A

ENTRYPOINT ["../docker-cmd.sh"]

