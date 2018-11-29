# T_T
Texture_Transcoder......Transcode texture data to GPU support format lossy in real-time.






### Emscripten
emcc -O3 TT/ETC.cpp -s WASM=0 -s EXPORTED_FUNCTIONS="['_malloc', '_free', '_TranscodeETC2_to_RGBA8', '_TranscodeETC2_EAC_to_RGBA8']" -s NO_EXIT_RUNTIME=1 -s NO_FILESYSTEM=1 -o --memory-init-file 0 -s ALLOW_MEMORY_GROWTH=1 tt.js
