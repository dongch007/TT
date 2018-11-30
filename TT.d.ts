declare module Module {
    var HEAPU8:  Uint8Array;
    function _malloc(size: number): number;
    function _free(ptr: number): void;

    function _TranscodeETC2_to_RGBA8(source: number, dest: number, width: number, height: number): void;
    function _TranscodeETC2_EAC_to_RGBA8(source: number, dest: number, width: number, height: number): void;
}