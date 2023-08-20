FROM archlinux
RUN printf '[multilib]\nInclude = /etc/pacman.d/mirrorlist\n' >> /etc/pacman.conf
RUN pacman -Syyu --noconfirm cmake ninja gcc gcc-multilib lib32-libjpeg-turbo lib32-libpng lib32-sdl2 lib32-glibc lib32-mesa boost gdb
WORKDIR /app
COPY . .
RUN mkdir build
WORKDIR /app/build
RUN cmake -DCMAKE_GENERATOR=Ninja -DCMAKE_BUILD_TYPE=Debug ..
RUN ninja
RUN mkdir /tasquake
COPY content /tasquake/
RUN cp TASQuakeSim /tasquake/
WORKDIR /tasquake
CMD tail -f /dev/null

