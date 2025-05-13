/******************************************************************************
 * The Clear BSD License
 * Copyright (c) 2024 Dolby Laboratories
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   - Neither the name of Dolby Laboratories nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

package com.dolby.capture.filtersimulation;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.util.Log;
import android.util.Range;
import android.view.Display;
import android.view.Surface;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.stream.Collectors;

/**
 * For selecting a decoder given a video file.
 * Instead of providing hard-coded mappings from mime-type and profile to
 * a decoder, this class searches the component store and returns a list of valid codecs
 * that are able to play back the given video.
 */
public final class CodecBuilderImpl implements CodecBuilder {

    public static final String HEVC = "HEVC";
    public static final String AVC = "AVC";
    public static final String DV_ME = "DV-ME";
    private static final String DOLBY_VISION_TYPE = MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION;
    private static final String REFLEXIVE_DOLBY_PROFILE_LOOKUP = "DolbyVisionProfile";
    private static final int TOP_RESULT = 0;
    private final MediaExtractor extractor;
    private final MediaMetadataRetriever retriever;
    private final Context context;
    private MediaFormat decoderFormat;
    private final String outputFormat;

    public static final String DOLBY_GPU_DECODER = "c2.dolby.decoder.hevc";
    public static final String DOLBY_GPU_DECODER_MTK_VPP = "c2.mtk.hevc.decoder";
    public static final String DOLBY_DPU_DECODER = "c2.qti.dv.decoder";
    // TODO Add MTK DPU decoder.

    public static enum PlatformSupport
    {
        NONE,
        GPU,
        DPU
    };

    public static PlatformSupport getSupportStatus()
    {
        MediaCodecList mediaCodecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);

        ArrayList<PlatformSupport> supports = Arrays.stream(mediaCodecList.getCodecInfos()).filter(x -> {
           return x.getCanonicalName().equals(DOLBY_GPU_DECODER)
                   || x.getCanonicalName().equals(DOLBY_GPU_DECODER_MTK_VPP)
                   || x.getCanonicalName().equals(DOLBY_DPU_DECODER);
        }).map(x ->
        {
          switch (x.getCanonicalName())
          {
              case DOLBY_GPU_DECODER:
              case DOLBY_GPU_DECODER_MTK_VPP:
                  return PlatformSupport.GPU;
              case DOLBY_DPU_DECODER:
                  return PlatformSupport.DPU;
              default:
                  return PlatformSupport.NONE;
          }
        }).collect(Collectors.toCollection(ArrayList::new));

        if(supports.isEmpty())
        {
            return PlatformSupport.NONE;
        }
        else
        {
            return supports.get(0);
        }
    }

    /**
     * Checks whether a given decoder name matches the name of the Dolby Vision GPU decoder.
     *
     * @param decoderName The name of the decoder.
     * @return True if the provided decoder is the Dolby Vision GPU decoder, false otherwise.
     */
    public boolean isDolbyDecoder(String decoderName) {
        return decoderName.equals(DOLBY_GPU_DECODER)
                || decoderName.equals(DOLBY_DPU_DECODER)
                || (decoderName.equals(DOLBY_GPU_DECODER_MTK_VPP) && decoderFormat.getString(MediaFormat.KEY_MIME) == "video/dolby-vision");
    }

    /**
     * Checks whether a given decoder name matches the name of the Dolby Vision DPU decoder.
     *
     * @param decoderName The name of the decoder.
     * @return True if the provided decoder is the Dolby Vision DPU decoder, false otherwise.
     */
    public static boolean isDolbyDPUDecoder(String decoderName) {
        return decoderName.equals(DOLBY_DPU_DECODER);
    }


    /**
     * Constructor. Simply set up the extractor to point at the file provided by the user.
     *
     * @param context The Android context.
     * @param uri     The URI for the file.
     * @throws IOException File cannot be opened.
     */
    public CodecBuilderImpl(Context context, String outputFormat, Uri uri) throws IOException {

        this.extractor = new MediaExtractor();
        this.extractor.setDataSource(context, uri, null);

        this.retriever = new MediaMetadataRetriever();
        this.retriever.setDataSource(context, uri);

        this.context = context;

        this.outputFormat = outputFormat;

    }

    /**
     * Checks for Dolby Vision support on the phone, this is a very high level check.
     *
     * @return True if the device is supportive, false otherwise.
     */
    public boolean isDolbySupported() {
        //Get the display and the corresponding abilities.
        Display screen = this.context.getDisplay();

        Display.HdrCapabilities capabilities = screen.getHdrCapabilities();

        //Check if the list of abilities is empty when anything that is not Dolby Vision is filtered.
        return !(Arrays.stream(capabilities.getSupportedHdrTypes()).
                filter(x -> x == Display.HdrCapabilities.HDR_TYPE_DOLBY_VISION).
                boxed().
                collect(Collectors.toCollection(ArrayList::new)).isEmpty());
    }

    /**
     * Maps the tracks present in a file to a track ID number and its format.
     *
     * @return A hashmap matching the above description.
     */
    private HashMap<Integer, MediaFormat> enumerateTracks() {

        HashMap<Integer, MediaFormat> formats = new HashMap<>();

        //Map all tracks in the file.
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            //The key is the track number which is mapped to the format of that track.
            formats.put(i, extractor.getTrackFormat(i));
        }

        return formats;
    }

    /**
     * Returns a list of tracks which are of a certain mime prefix.
     * Specifically if codec type is VIDEO then all tracks with a mime type starting with video
     * will be returned. The method is not case sensitive, both video and Video will match.
     * The given mime type is converted to lowercase before comparison.
     * A list of media format objects that represent tracks
     *
     * @param type A flag for the type of track.
     * @return A list of tracks that are of the requested type.
     */
    private List<Track> getTracksByType(CodecType type) {

        //For every track in the file
        return this.enumerateTracks().entrySet().stream().filter(x -> {

                    //Ensure the format for that track is present.
                    if (x.getValue() != null) {

                        //check if the mime type starts with the desired prefix, video or audio.
                        return x.getValue().getString(MediaFormat.KEY_MIME).toLowerCase(Locale.ROOT).startsWith(type == CodecType.VIDEO ? "video" : "audio");

                    } else {
                        //If format is not present.
                        return false;
                    }
                    //Take the output and create tracks from them.
                }).map(Track::new)
                .collect(Collectors.toCollection(ArrayList::new));
    }

    /**
     * Arm the extractor to the track that matches the chosen codec.
     * We assume there will only be 1 video track in the file.
     *
     * @param type Flag to allow other formats in future. For now always video.
     */
    private void armExtractor(CodecType type) throws NoCodecException {
        if (type == CodecType.VIDEO) {
            this.extractor.selectTrack(this.getPlayableTrack().getId());
        }
    }

    public int getFrameRate() throws NoCodecException {
        int trackID = this.getPlayableTrack().getId();

        return this.extractor.getTrackFormat(trackID).getInteger(MediaFormat.KEY_FRAME_RATE);
    }

    /**
     * Find the video track in the file and return its format.
     *
     * @return The track format.
     */
    private Track getPlayableTrack() throws NoCodecException {

        //Get a list of video tracks in the file.
        List<Track> videoTracks = this.getTracksByType(CodecType.VIDEO);

        AtomicBoolean option = new AtomicBoolean(false);

        Log.d("GetPlayableTrack", "getPlayableTrack: Blocking non HEVC Codecs " + this.outputFormat);
        ArrayList<Track> tracks = videoTracks.parallelStream().filter(x -> {

                    //See if there are any Dolby tracks in the file, this filtering only applies to this case.
                    boolean isDolby = videoTracks.parallelStream().anyMatch(f -> f.getFormat().getString(MediaFormat.KEY_MIME).equals(MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION) &&
                            f.getFormat().getInteger(MediaFormat.KEY_PROFILE) == MediaCodecInfo.CodecProfileLevel.DolbyVisionProfileDvheSt);

                    if (isDolby) {
                        //Set the decoder for an HEVC output.
                        if (outputFormat.equals(HEVC) || outputFormat.equals(AVC) && !option.get()) {
                            //Eliminate any tracks that are not Dolby Vision.
                            boolean found = x.getFormat().getString(MediaFormat.KEY_MIME).equals(MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION);
                            if (found) {
                                option.set(true);
                            }

                            return found;
                        }
                        //Set the decoder for Dolby Vision output.
                        else if (outputFormat.equals(DV_ME) && !option.get()) { //
                            //Eliminate any tracks that are not HEVC (really Dolby Vision base layer.
                            boolean found = x.getFormat().getString(MediaFormat.KEY_MIME).equals(MediaFormat.MIMETYPE_VIDEO_HEVC);

                            if (found) {
                                option.set(true);
                            }

                            return found;

                        } else {
                            return false;
                        }
                    } else {
                        return true;
                    }
                })
                .filter(t -> this.filterCodecByFormat(t.getFormat()) != null)
                .collect(Collectors.toCollection(ArrayList::new));
        Log.e("GetPlayableTrack", "getPlayableTrack: list " + Arrays.toString(tracks.toArray()));

        if (tracks.isEmpty()) {
            //The device cannot play the file.
            throw new NoCodecException();
        } else {
            return tracks.get(TOP_RESULT);
        }
    }

    /**
     * Take the list of codecs available on the phone and return a list of only available Dolby codecs.
     *
     * @param dolbyCodecsOnly Set to false to return all available codecs.
     * @return A list of codecs filtered accordingly.
     */
    private MediaCodecInfo[] nonDolbyKnockout(final boolean dolbyCodecsOnly) {
        //In order to take advantage of reflection APIs we need an object to operate on.
        final MediaCodecInfo.CodecProfileLevel reflexiveReference = new MediaCodecInfo.CodecProfileLevel();

        /*
        This operation reflexively asks the object defined above to return a list of all its
        declared fields. This returns a list of all class variables, regardless of access level.
        We then get the name of these and only keep the ones that start with Google's defined prefix
        for Dolby Vision profiles. The integer set returned has these values in it.
         */
        Set<Integer> dolbyProfiles = Arrays.stream(MediaCodecInfo.CodecProfileLevel.class.getDeclaredFields())
                //Filter for only Dolby Vision profiles.
                .filter(x -> x.getName().startsWith(REFLEXIVE_DOLBY_PROFILE_LOOKUP))
                .map(x -> {
                    try {
                        //Map the variable name to its value.
                        return x.getInt(reflexiveReference);
                    } catch (IllegalAccessException e) {
                        e.printStackTrace();
                    }
                    return null;
                })
                //Return a list of those values.
                .collect(Collectors.toSet());

        /*
        Go through the available codecs on the phone and filter out the ones that do not support
        at least one of the profiles we found above.
         */
        return Arrays.stream(codecInfos).
                //We are not interested in encoders.
                        filter(x -> !x.isEncoder()).
                filter(x -> {

                    //High level check to see if the codec supports the Dolby Vision mime type.
                    if (Arrays.asList(x.getSupportedTypes()).contains(DOLBY_VISION_TYPE)) {

                        //Check for profile support.
                        for (MediaCodecInfo.CodecProfileLevel profLevel : x.getCapabilitiesForType(DOLBY_VISION_TYPE).profileLevels) {

                            if (dolbyProfiles.contains(profLevel.profile) && dolbyCodecsOnly) {
                                return true;
                            }
                        }
                    }

                    return !dolbyCodecsOnly;

                }).toArray(MediaCodecInfo[]::new);

    }

    /**
     * A utility function for getting codecs for a Dolby Vision Profile.
     *
     * @return A hashmap that returns profile numbers mapped to a list of codecs.
     */
    public HashMap<Integer, ArrayList<String>> getDolbyCodecMapping() {
        //Hashmap that we will ultimately return.
        HashMap<Integer, ArrayList<String>> dolbyProfileToCodecLookup = new HashMap<>();

        //Get a list of Dolby codecs on the phone.
        Arrays.stream(nonDolbyKnockout(true)).
                filter(c -> !c.isEncoder()).
                filter(c -> !c.getCapabilitiesForType(DOLBY_VISION_TYPE).
                        isFeatureSupported(MediaCodecInfo.CodecCapabilities.FEATURE_SecurePlayback)).
                filter(codec ->
                        Arrays.asList(codec.getSupportedTypes()).contains(DOLBY_VISION_TYPE)).
                forEach(c -> Arrays.stream(c.getCapabilitiesForType(DOLBY_VISION_TYPE).profileLevels).forEach(prof -> {

                    if (!dolbyProfileToCodecLookup.containsKey(prof.profile)) {
                        ArrayList<String> value = new ArrayList<>();
                        value.add(c.getCanonicalName());
                        dolbyProfileToCodecLookup.put(prof.profile, value);
                    } else {
                        Objects.requireNonNull(dolbyProfileToCodecLookup.get(prof.profile)).add(c.getCanonicalName());
                    }
                }));

        return dolbyProfileToCodecLookup;
    }

    public String printDolbyCodecSupport() {

        final MediaCodecInfo.CodecProfileLevel reflexiveReference = new MediaCodecInfo.CodecProfileLevel();

        StringBuilder builder = new StringBuilder();

        HashMap<Integer, ArrayList<String>> profileMap = getDolbyCodecMapping();

        Field[] codecInfoFields = MediaCodecInfo.CodecProfileLevel.class.getDeclaredFields();

        //Get a list of supported Dolby profile names.
        ArrayList<Field> fields = Arrays.stream(codecInfoFields).filter(f -> {
            try {
                return profileMap.containsKey(f.getInt(reflexiveReference)) && f.getName().startsWith(REFLEXIVE_DOLBY_PROFILE_LOOKUP);
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            }
            return false;
        }).collect(Collectors.toCollection(ArrayList::new));

        for (int count = 0; count < fields.size(); count++) {
            try {
                builder.append(fields.get(count).getName()).append(" ").append(profileMap.get(fields.get(count).getInt(reflexiveReference))).append("\n");
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            }
        }

        return builder.toString();

    }


    /**
     * Given a track format find a decoder that is able to play it.
     *
     * @param format The format of the track we are trying to play.
     * @return A codec name, or null if the track is not playable.
     */
    private String filterCodecByFormat(MediaFormat format) {

        //Build a format with the requirements a candidate codec will need to meet.
        MediaFormat targetFormat = new MediaFormat();
        targetFormat.setString(MediaFormat.KEY_MIME, format.getString(MediaFormat.KEY_MIME));
        targetFormat.setInteger(MediaFormat.KEY_WIDTH, format.getInteger(MediaFormat.KEY_WIDTH));
        targetFormat.setInteger(MediaFormat.KEY_HEIGHT, format.getInteger(MediaFormat.KEY_HEIGHT));
        targetFormat.setInteger(MediaFormat.KEY_PROFILE, format.getInteger(MediaFormat.KEY_PROFILE));
//        targetFormat.setInteger(MediaFormat.KEY_LEVEL, format.getInteger(MediaFormat.KEY_LEVEL));
        targetFormat.setFloat(MediaFormat.KEY_FRAME_RATE, format.getInteger(MediaFormat.KEY_FRAME_RATE));
        //targetFormat.setInteger(MediaFormat.KEY_BIT_RATE, Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE)));
        MediaCodecList mediaCodecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        String decoder = mediaCodecList.findDecoderForFormat(targetFormat);
        return decoder;
    }


    public MediaFormat getDecodedFormat() {
        return this.decoderFormat;
    }


    @Override
    public MediaExtractor configure(MediaCodec codec, Surface surface, Codec c, int transfer) throws NoCodecException {
        MediaFormat format = this.getPlayableTrack().format;

        this.decoderFormat = format;

        String mime = format.getString(MediaFormat.KEY_MIME);

        if(getSupportStatus() == PlatformSupport.DPU) {

            if ((decoderFormat.getString(MediaFormat.KEY_MIME).equals(MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION)) &&
                    (decoderFormat.getInteger(MediaFormat.KEY_PROFILE) == MediaCodecInfo.CodecProfileLevel.DolbyVisionProfileDvheSt) &&
                    (outputFormat.equals(HEVC) || outputFormat.equals(AVC))) {
                Log.e("CodecSelection", "configure: Dolby Vision Decoder Filter Enabled");
                format.setInteger(MediaFormat.KEY_COLOR_TRANSFER_REQUEST, MediaFormat.COLOR_TRANSFER_SDR_VIDEO);
            } else {
                Log.e("CodecSelection", "configure: Not enabling Dolby Vision Filter");
            }

        }
        else {
            if (mime.equals(MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION)) {
                c.setTransfer(Codec.TRANSFER_PARAMS[transfer]);
            }
        }

        codec.configure(format, surface, null, 0);

        return this.extractor;
    }

    @Override
    public String getCodecName() throws IOException, NoCodecException {

        String codecName;
        do {
            codecName = this.filterCodecByFormat(this.getPlayableTrack().format);
        }
        while (codecName == null);

        this.armExtractor(CodecType.VIDEO);

        return codecName;
    }

    private static class Track {
        private final int id;
        private final MediaFormat format;

        public Track(Map.Entry<Integer, MediaFormat> entry) {
            this.id = entry.getKey();
            this.format = entry.getValue();
        }

        public int getId() {
            return id;
        }

        public MediaFormat getFormat() {
            return format;
        }

        @Override
        public String toString() {
            return id + " " + format + " " + format.getString(MediaFormat.KEY_MIME);
        }
    }

    public static class NoCodecException extends Exception {
        public NoCodecException() {
            super("No available codec.");
        }
    }
}
