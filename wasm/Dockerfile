FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libreadline-dev \
    libncurses-dev \
    libbsd-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY Libft/ Libft/
COPY srcs/ srcs/
COPY includes/ includes/
COPY Makefile .

RUN make CFLAGS="-Iincludes -ILibft/includes -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -DLIBBSD_OVERLAY -isystem /usr/include/bsd -Wall -Wextra -Werror -MD -MP" LDFLAGS="-LLibft -lft -lreadline -ltermcap -lbsd" \
    && cp 42sh /usr/local/bin/42sh

RUN make debug CFLAGS="-Iincludes -ILibft/includes -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -DLIBBSD_OVERLAY -isystem /usr/include/bsd -Wall -Wextra -Werror -MD -MP" LDFLAGS="-LLibft -lft -lreadline -ltermcap -lbsd" \
    && cp 42sh /usr/local/bin/42sh_debug

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libreadline8 \
    libtinfo6 \
    libbsd0 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/bin/42sh /bin/42sh
COPY --from=builder /usr/local/bin/42sh_debug /bin/42sh_debug

ENV HOME=/root
ENV TERM=xterm-256color
ENV SH42_VIZ_DIR=/var/log/42sh
RUN mkdir -p /var/log/42sh
WORKDIR /root

ENTRYPOINT ["/bin/42sh", "-i"]
