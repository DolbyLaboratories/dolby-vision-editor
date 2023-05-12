# Release notes


## What's new in this release

This is the first release of the Dolby Vision editor sample application.

## Supported features

This version of the sample application supports features including:
- GPU or DPU implementations for display management
- Applying filters for saturation, contrast, text overlays, etc.
- Previewing and HLG signaling.
- Decoding, encoding, and transcoding to or from formats including HEVC, AVC, and Dolby Vision 8.4.
- Trimming and re-multiplexing.


## Known limitations/issues

1. The platform SOC must support encoding to P010 format in order to generate Dolby Vision profile 8.4 content.
2. Some OEM platforms may not provide stable support for this application.
3. Some OEM platforms may not provide correct coefficients in Video Usability Information (VUI).
4. The "lossless" feature for "I-FRAME INTERVAL" may introduce noise in some videos.
