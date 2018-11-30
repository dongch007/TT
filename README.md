# T_T
Texture_Transcoder......Transcode texture data to GPU support format lossy in real-time.






### Emscripten
emcc -O3 TT/ETC.cpp -s WASM=0 -s EXPORTED_FUNCTIONS="['_malloc', '_free', '_TranscodeETC2_to_RGBA8', '_TranscodeETC2_EAC_to_RGBA8']" -s NO_EXIT_RUNTIME=1 -s NO_FILESYSTEM=1 --memory-init-file 0 -s ALLOW_MEMORY_GROWTH=1 -o tt.js
```ts
  let sourcePtr  = Module._malloc(sourceSize);
  Module.HEAPU8.set(sourceData, sourcePtr);

  let destSize = Math.max(width*height*4, 64)
  let destPtr = Module._malloc(destSize);
  
  Module._TranscodeETC2_EAC_to_RGBA8(sourcePtr, destPtr, width, height);

  let uncompressedData = new Uint8Array(Module.HEAPU8.buffer, destPtr, destSize);
  
  Module._free(sourcePtr);
  Module._free(destPtr);
```
