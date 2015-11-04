#!/bin/bash

case $CIRCLE_NODE_INDEX in 0)
       docker run -v $HOME/docker-shared/:/home/build/shared jpf91/build-gdc /usr/bin/build-gdc build --toolchain=x86_64-linux-gnu/gcc-4.8/devkitPPC --keep-alive=60
   ;; 1) 
       docker run -v $HOME/docker-shared/:/home/build/shared jpf91/build-gdc /usr/bin/build-gdc build --toolchain=x86_64-linux-gnu/gcc-4.9/devkitARM --keep-alive=60
   ;; 2)
   ;; 3)
;; esac

#case $CIRCLE_NODE_INDEX in 0)
#       docker run -v $HOME/docker-shared/:/home/build/shared jpf91/build-gdc /usr/bin/build-gdc build --toolchain=x86_64-linux-gnu/gcc-4.8/devkitPPC --toolchain=i686-w64-mingw32/gcc-4.8/devkitPPC --keep-alive=60
#   ;; 1) 
#       docker run -v $HOME/docker-shared/:/home/build/shared jpf91/build-gdc /usr/bin/build-gdc build --toolchain=x86_64-linux-gnu/gcc-4.9/devkitARM --toolchain=i686-w64-mingw32/gcc-4.9/devkitARM --keep-alive=60
#   ;; 2)
#       docker run -v $HOME/docker-shared/:/home/build/shared jpf91/build-gdc /usr/bin/build-gdc build --toolchain=i686-linux-gnu/gcc-4.9/devkitARM --keep-alive=60
#   ;; 3)
#       docker run -v $HOME/docker-shared/:/home/build/shared jpf91/build-gdc /usr/bin/build-gdc build --toolchain=i686-linux-gnu/gcc-4.8/devkitPPC --keep-alive=60
#;; esac
