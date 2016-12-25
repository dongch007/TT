# T_T
Texture_Transcoder......Transcode kinds of annoying texture data to GPU support format lossy in real-time.

For example on android device, there are so many gpu support compress texture format:
          
          Tegra/Vivante-—----DXTC
          Adreno-------------ATC
          PowerVR------------PVRTC
          GLES3--------------ETC2/ASTC
          GLES2--------------ETC1
          
For texture no alpha channel, we can convert textures to ETC1 offline, all devices > gles2 will support. 

If texture has alpha channel:

          use RGBA8---------------------------------------too many memory and bandwidth
          use RGBA4---------------------------------------too many loss
          use ETC1 for RGB, the other ETC1/A8 for alpha---sample two textures in shader
          use DXT5/ATC_RGBA/EAC/....----------------------only part of devices support, others decode to RGBA8

This project is use for the fourth status.

Assume select DXT5 as default format.

          if support GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
              do nothing
          else if support GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD
              transcode dxt5 to atc
          else if support GL_COMPRESSED_RGBA8_ETC2_EAC
              transcode dxt5 to eac    
          else if memory limited
              transcode dxt5 to RGBA4
          else
              transcode dxt5 to RGBA8
 
 
 








##Reference
[A Method for Load-Time Conversion of DXTC Assets to ATC](http://www.guildsoftware.com/papers/2012.Converting.DXTC.to.ATC.pdf)
