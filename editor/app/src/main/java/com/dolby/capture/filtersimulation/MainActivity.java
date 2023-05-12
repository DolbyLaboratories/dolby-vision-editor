/******************************************************************************
 * The Clear BSD License
 * Copyright (c) 2023 Dolby Laboratories
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

import android.Manifest;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.media.MediaCodec;
import android.net.Uri;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.WindowMetrics;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.google.android.material.slider.Slider;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Comparator;
import java.util.stream.IntStream;

public class MainActivity extends Activity implements View.OnClickListener, AdapterView.OnItemSelectedListener, SurfaceHolder.Callback, Comparator<String>, BroadcastClient {

    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int REQUEST_FOR_VIDEO_FILE = 1000;

    private static final int TRANSFER_DOLBY = 3;
    private static final int TRANSFER_SDR = 0;

    private ContentLoader p = null;

    private Surface screenSurface;

    private Size screen;

    private boolean playPauseVisible = false;

    // Declaring all elements in xml
    private Button btnStartExport;
    private ImageButton btnSelectNewVideo, btnPlay, btnPause, btnExport, btnCloseExportPanel, btnResetEffects, btnResetOverlayText;
    private Spinner spinnerFilterType, spinnerTextColor, spinnerResolutionVal, spinnerEncoderFormat, spinnerIFrameInterval;
    private Slider sliderGainValue, sliderOffsetValue, sliderContrastValue, sliderSaturationValue, sliderWiperValue, sliderOpacityValue;
    private SwitchCompat switchZebra, switchGamut, switchTrimOnly;
    private EditText simpleEditText;
    private RadioGroup rgTextPosition;
    private SurfaceView preview;
    private View clSelectVideoBar, progressOverlayView, dimControlsView, dimScreenView, exportOverlayView;
    private String resolution;
    private String encoderFormat;
    private int iFrameInterval;
    //If the default output format is changed be sure to update this variable accordingly!
    private int transfer = TRANSFER_SDR;

    private boolean trimOnly = false;

    private String inputPath;
    private Uri inputUri;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate");
        super.onCreate(savedInstanceState);


        setTheme(R.style.DarkTheme);
        setContentView(R.layout.activity_main);

        requestPermissions();

        // Initializing all xml elements
        btnStartExport = findViewById(R.id.btn_start_video_export);
        btnSelectNewVideo = (ImageButton) findViewById(R.id.btn_select_new_video);
        btnPlay = (ImageButton) findViewById(R.id.btn_play);
        btnPause = (ImageButton) findViewById(R.id.btn_pause);
        btnExport = (ImageButton) findViewById(R.id.btn_export);
        btnCloseExportPanel = (ImageButton) findViewById(R.id.btn_close_export_panel);
        btnResetEffects = (ImageButton) findViewById(R.id.btn_reset_effects);
        btnResetOverlayText = (ImageButton) findViewById(R.id.btn_reset_overlay_text);
        spinnerFilterType = findViewById(R.id.spinner_filter_type);
        spinnerTextColor = findViewById(R.id.spinner_text_color);
        spinnerResolutionVal = findViewById(R.id.spinner_resolution_val);
        spinnerEncoderFormat = findViewById(R.id.spinner_encoder_format);
        spinnerIFrameInterval = findViewById(R.id.spinner_iframe_interval);
        sliderGainValue = findViewById(R.id.gainSlider);
        sliderOffsetValue = findViewById(R.id.offsetSlider);
        sliderContrastValue = findViewById(R.id.contrastSlider);
        sliderSaturationValue = findViewById(R.id.saturationSlider);
        sliderWiperValue = findViewById(R.id.wiperSlider);
        sliderOpacityValue = findViewById(R.id.opacitySlider);
        switchZebra = (SwitchCompat) findViewById(R.id.switch_zebra);
        switchGamut = (SwitchCompat) findViewById(R.id.switch_gamut);
        switchTrimOnly = findViewById(R.id.switch_trimOnly);
        progressOverlayView = findViewById(R.id.progress_overlay);
        preview = findViewById(R.id.surfaceView);
        clSelectVideoBar = findViewById(R.id.cl_select_video_bar);
        simpleEditText = findViewById(R.id.simpleEditText);
        rgTextPosition = findViewById(R.id.radioGroupTextPosition);
        dimControlsView = findViewById(R.id.dim_controls);
        dimScreenView = findViewById(R.id.dim_screen);
        exportOverlayView = findViewById(R.id.export_overlay);

        populateSpinners();

        preview.getHolder().addCallback(this);

        preview.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (playPauseVisible || p == null) {
                    btnPlay.setVisibility(View.INVISIBLE);
                    btnPause.setVisibility(View.INVISIBLE);
                    playPauseVisible = false;
                } else {
                    btnPlay.setVisibility(View.VISIBLE);
                    btnPause.setVisibility(View.VISIBLE);
                    playPauseVisible = true;
                }
            }
        });

        // set listener for buttons
        clSelectVideoBar.setOnClickListener(this);
        btnSelectNewVideo.setOnClickListener(this);
        btnStartExport.setOnClickListener(this);
        btnPlay.setOnClickListener(this);
        btnPause.setOnClickListener(this);
        btnExport.setOnClickListener(this);
        btnCloseExportPanel.setOnClickListener(this);
        dimScreenView.setOnClickListener(this);

        sliderGainValue.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                if (p != null) {
                    p.EditShadersSetParameter(EffectParameters.GAIN.ordinal(), value * 0.01f);
                }
            }
        });

        sliderOffsetValue.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                if (p != null) {
                    p.EditShadersSetParameter(EffectParameters.OFFSET.ordinal(), value * 0.01f);
                }
            }
        });

        sliderContrastValue.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                if (p != null) {
                    p.EditShadersSetParameter(EffectParameters.CONTRAST.ordinal(), value * 0.01f);
                }
            }
        });

        sliderSaturationValue.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                if (p != null) {
                    p.EditShadersSetParameter(EffectParameters.SATURATION.ordinal(), value * 0.01f);
                }
            }
        });

        sliderWiperValue.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                if (p != null) {
                    p.EditShadersSetParameter(EffectParameters.WIPER_LEFT.ordinal(), value * 0.01f);
                }
            }
        });

        spinnerFilterType.setOnItemSelectedListener(this);
        spinnerResolutionVal.setOnItemSelectedListener(this);
        spinnerEncoderFormat.setOnItemSelectedListener(this);
        spinnerIFrameInterval.setOnItemSelectedListener(this);

        switchZebra.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (p != null) {
                    if (isChecked) {
                        p.EditShadersSetParameter(EffectParameters.ZEBRA_ENABLE.ordinal(), 1.0f);
                    } else {
                        p.EditShadersSetParameter(EffectParameters.ZEBRA_ENABLE.ordinal(), 0.0f);
                    }
                }
            }
        });

        switchGamut.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (p != null) {
                    if (isChecked) {
                        p.EditShadersSetParameter(EffectParameters.GAMUT_ENABLE.ordinal(), 1.0f);
                    } else {
                        p.EditShadersSetParameter(EffectParameters.GAMUT_ENABLE.ordinal(), 0.0f);
                    }
                }
            }
        });

        switchTrimOnly.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                trimOnly = isChecked;
            }
        });

        simpleEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                applyTextCompositing();
            }
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override
            public void afterTextChanged(Editable s) {}
        });

        spinnerTextColor.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                applyTextCompositing();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        sliderOpacityValue.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                applyTextCompositing();
            }
        });

        rgTextPosition.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                applyTextCompositing();
            }
        });

        btnResetEffects.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG, "Reset Effects");
                Toast.makeText(getApplicationContext(), "Reset the effects", Toast.LENGTH_SHORT).show();
                resetEffects();
            }
        });

        btnResetOverlayText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG, "Reset Text Overlay");
                Toast.makeText(getApplicationContext(), "Reset the text overlay", Toast.LENGTH_SHORT).show();
                resetTextOverlay();
            }
        });

        disableControls();


        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        this.screen = this.setVideoSize(new Size(1920, 1080),0);

    }

    private void resetEffects() {
        sliderGainValue.setValue(100.0f);
        sliderOffsetValue.setValue(0.0f);
        sliderContrastValue.setValue(100.0f);
        sliderSaturationValue.setValue(100.0f);
        sliderWiperValue.setValue(0.0f);
        switchZebra.setChecked(false);
        switchGamut.setChecked(false);
        spinnerFilterType.setSelection(0);
    }

    private void resetTextOverlay() {
        simpleEditText.setText("");
        spinnerTextColor.setSelection(0);
        sliderOpacityValue.setValue(0.5f);
        View radioButton = rgTextPosition.getChildAt(0);
        rgTextPosition.check(radioButton.getId());
    }

    private void populateSpinners() {
        Log.i(TAG, "populateSpinners()");

        ArrayAdapter<String> adapterFilterType= new ArrayAdapter<String>(this, R.layout.custom_spinner_item, Constants.FILTER_OPTIONS);
        adapterFilterType.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerFilterType.setAdapter(adapterFilterType);

        ArrayAdapter<String> adapterTextColor = new ArrayAdapter<>(this, R.layout.custom_spinner_item, Constants.TEXT_COLORS);
        adapterTextColor.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerTextColor.setAdapter(adapterTextColor);

        ArrayAdapter<String> adapterResolutionVal= new ArrayAdapter<String>(this, R.layout.custom_spinner_item, Constants.RESOLUTION_OPTIONS);
        adapterResolutionVal.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerResolutionVal.setAdapter(adapterResolutionVal);

        ArrayAdapter<String> adapterEncoderFormat= new ArrayAdapter<String>(this, R.layout.custom_spinner_item, Constants.ENCODER_FORMAT);
        adapterEncoderFormat.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerEncoderFormat.setAdapter(adapterEncoderFormat);

        ArrayAdapter<String> adapterIFrameInterval = new ArrayAdapter<>(this, R.layout.custom_spinner_item, Constants.IFRAME_INTERVALS);
        adapterIFrameInterval.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerIFrameInterval.setAdapter(adapterIFrameInterval);

        // Set all the spinners and switches to default values
        spinnerFilterType.setSelection(0);
        spinnerTextColor.setSelection(0);
        spinnerResolutionVal.setSelection(0);
        spinnerEncoderFormat.setSelection(0);
        spinnerIFrameInterval.setSelection(1);
    }

    private void enableControls() {
        dimControlsView.setVisibility(View.INVISIBLE);
        clSelectVideoBar.setVisibility(View.INVISIBLE);
        btnSelectNewVideo.setVisibility(View.VISIBLE);
        btnExport.setAlpha(1.0f);
        btnExport.setEnabled(true);
    }

    private void disableControls() {
        dimControlsView.setVisibility(View.VISIBLE);
        clSelectVideoBar.setVisibility(View.VISIBLE);
        btnSelectNewVideo.setVisibility(View.INVISIBLE);
        btnExport.setAlpha(0.3f);
        btnExport.setEnabled(false);
    }

    private void showExportPanel() {
        dimScreenView.setVisibility(View.VISIBLE);
        exportOverlayView.setVisibility(View.VISIBLE);
    }

    private void hideExportPanel() {
        dimScreenView.setVisibility(View.INVISIBLE);
        exportOverlayView.setVisibility(View.INVISIBLE);
    }

    private void setFilter(String filter) {
        Log.e(TAG, "setFilter()");
        if (p == null) {
            return;
        }

        switch (filter) {
            case Constants.FILTER_BW:
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_Y.ordinal(), -1.0f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_U.ordinal(), 0.5f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_V.ordinal(), 0.5f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_A.ordinal(), -1.0f);
                break;
            case Constants.FILTER_SEPIA:
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_Y.ordinal(), -1.0f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_U.ordinal(), 0.45f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_V.ordinal(), 0.55f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_A.ordinal(), -1.0f);
                break;
            default:
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_Y.ordinal(), -1.0f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_U.ordinal(), -1.0f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_V.ordinal(), -1.0f);
                p.EditShadersSetParameter(EffectParameters.OVERRIDE_A.ordinal(), -1.0f);
                break;
        }
    }

    private void errorDialog(String title, String message) {
        // setup the alert builder
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(title);
        builder.setMessage(message);

        // add a button
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                MainActivity.this.runOnUiThread(() -> {
                    recreate();
//                    spinnerEncoderFormat.setSelection(0);
                });
            }
        });

        // create and show the alert dialog
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    @Override
    public void onClick(View v) {
        Log.d(TAG, "Click!");
        int id = v.getId();
        if (id == R.id.cl_select_video_bar || id == R.id.btn_select_new_video) {

            // Allow user to choose one video file
            Intent selectVideoIntent = new Intent();
            selectVideoIntent.setType("video/*");
            selectVideoIntent.setAction(Intent.ACTION_GET_CONTENT);
            startActivityForResult(selectVideoIntent, REQUEST_FOR_VIDEO_FILE);
            resetEffects();
            resetTextOverlay();

        } else if (id == R.id.btn_start_video_export) {

            if (inputPath == null) {
                // Inform user to select a file before decoding
                Log.d(TAG, "onClick: inputPath " + inputPath);
                Toast.makeText(this, "Select a video file before exporting", Toast.LENGTH_SHORT).show();
            } else {
                try {
                    animateView(progressOverlayView, View.VISIBLE, 0.4f, 200);
                    p.EditShadersSetParameter(EffectParameters.WIPER_LEFT.ordinal(), 0.0f);
                    if (trimOnly) {
                        p.load_trim(inputUri, screenSurface, this, this.screen);
                    } else {
                        p.load_transcode(this, inputUri, this, this.screen, resolution, encoderFormat, transfer, iFrameInterval);
                    }

                } catch (Exception e) {
                    e.printStackTrace();
                    errorDialog("ERROR", e.getMessage());
                }
            }

        } else if (id == R.id.btn_play) {

            if (p != null) {
                p.play();
            }

        } else if (id == R.id.btn_pause) {

            if (p != null) {
                p.pause();
            }

        } else if (id == R.id.btn_export) {

            showExportPanel();

        } else if (id == R.id.btn_close_export_panel || id == R.id.dim_screen) {

            hideExportPanel();

        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "OnActivityResult!");
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_FOR_VIDEO_FILE && resultCode == RESULT_OK) {
            if (data != null && data.getData() != null) {
                inputUri = data.getData();
                inputPath = data.getData().toString();
                Log.i(TAG, "Video file selected Input path:" + inputPath);
                enableControls();
            }
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        String item = parent.getItemAtPosition(position).toString();
        switch (parent.getId()) {
            case R.id.spinner_filter_type:
                setFilter(item);
                break;

            case R.id.spinner_resolution_val:
                resolution = item;
                break;

            case R.id.spinner_encoder_format:
                switch (item) {
                    case Constants.ENCODER_HEVC:
                        encoderFormat = Constants.HEVC;
                        break;
                    case Constants.ENCODER_AVC:
                        encoderFormat = Constants.AVC;
                        break;
                    case Constants.ENCODER_P84:
                        encoderFormat = Constants.DV_ME;
                        break;
                }
                if (this.p != null) {
                    if (encoderFormat.equals(Constants.DV_ME)) {
                        transfer = TRANSFER_DOLBY;
                    } else {
                        transfer = TRANSFER_SDR;
                    }

                    try {
                        p.load_preview(inputUri, screenSurface, this, this.screen, encoderFormat, transfer, true);
                    } catch (MediaFormatNotFoundInFileException | IOException e) {
                        e.printStackTrace();
                    }

                }
                break;

            case R.id.spinner_iframe_interval:
                switch (item) {
                    case Constants.IFRAME_LOSSLESS:
                        iFrameInterval = 0;
                        break;
                    case Constants.IFRAME_1_SEC:
                        iFrameInterval = 1;
                        break;
                }
                break;
        }
    }
    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG, "onResume: ");
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (p != null) {
            p.stop();
        }
        this.inputPath = null;

        Log.e(TAG, "onPause: ");
    }

    public Size setVideoSize(Size videoDimensions, int rotation) {
        // Get the dimensions of the video
        int videoWidth = videoDimensions.getWidth();
        int videoHeight = videoDimensions.getHeight();
        Log.e(TAG, "setVideoSize: " + rotation + " " + videoWidth  + " " + videoHeight);

        float videoProportion = 0;
        if (rotation == 0 || rotation == 180) {
            videoProportion = (float) videoWidth / (float) videoHeight;
        }
        if (rotation == 90 || rotation == 270) {
            videoWidth /= 1.5f;
            videoHeight /= 1.5f;
            videoProportion = ((float) videoHeight / (float) videoWidth);
        }

        // Get the width of the screen
        final WindowMetrics metrics = getWindowManager().getCurrentWindowMetrics();
        final Rect bounds = metrics.getBounds();
        int screenHeight = bounds.height();
        int screenWidth = bounds.width();
        Log.e(TAG, "setVideoSize: " +  + screenWidth  + " " + screenHeight);

        float screenProportion = 0;
        if (rotation == 0 || rotation == 180) {
            screenProportion = (float) screenWidth / (float) screenHeight;
        }
        if (rotation == 90 || rotation == 270) {
            screenWidth /= 1.5f;
            screenHeight /= 1.5f;
            screenProportion = ((float) screenHeight / (float) screenWidth);
        }

        // Get the SurfaceView layout parameters
        android.view.ViewGroup.LayoutParams lp = preview.getLayoutParams();
        Log.e(TAG, "setVideoSize: Change in params " + lp.width + " " + lp.height);

        if (videoProportion > screenProportion) {
            Log.e(TAG, "setVideoSize: BRANCH 1 " + lp.width + " " + lp.height);
            lp.width = screenWidth;
            lp.height = (int) ((float) screenWidth / videoProportion);

        } else {
            Log.e(TAG, "setVideoSize: BRANCH 2"  );
            lp.width = (int) (videoProportion * (float) screenHeight);
            lp.height = screenHeight;
        }

        // Commit the layout parameters
        preview.setLayoutParams(lp);

        return new Size(lp.width, lp.height);
    }

    private void render(Uri inputUri) throws MediaFormatNotFoundInFileException, IOException {

        if (p == null) {
            p = new ContentLoader(this);
        }

        p.stop();
        p.load_preview(inputUri, screenSurface, this, this.screen, encoderFormat, transfer, false);
    }

    private boolean requestPermissions() {
        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED && ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            ActivityCompat.requestPermissions(MainActivity.this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            return false;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode == 1) {
            if (IntStream.of(grantResults).noneMatch(x -> x == PackageManager.PERMISSION_DENIED)) {
                Log.i(TAG, "onRequestPermissionsResult: permissions granted");
                MainActivity.this.recreate();
            } else {
                Log.w(TAG, "onRequestPermissionsResult: user failed to accept all required permissions");
            }
        }
    }


    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        Log.v(TAG, "surfaceCreated");
        screenSurface = holder.getSurface();
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        Log.v(TAG, "surfaceChanged");
        try {
            if (this.inputUri != null) {
                render(this.inputUri);
            }
        } catch (MediaFormatNotFoundInFileException | IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        Log.v(TAG, "surfaceDestroyed");
    }

    @Override
    public int compare(String o1, String o2) {
        int string1Dot = o1.indexOf(".");
        int string2Dot = o2.indexOf(".");

        String s1 = (o1.substring(0, string1Dot));
        String s2 = (o2.substring(0, string2Dot));

        return Integer.valueOf(s1).compareTo(Integer.valueOf(s2));
    }

    @Override
    public void acceptMessage(Message<?> message) {
        if (message.getPayload() instanceof VideoDecoder && message.getTitle().equals("Done")) {
            Log.e(TAG, "VideoDecoder acceptMessage: " + message.getTitle());
            onEditDone(true);
        }
        else if (message.getPayload() instanceof TrimDecoder && message.getTitle().equals("Done")) {
            Log.e(TAG, "TrimDecoder acceptMessage: " + message.getTitle());
            onEditDone(true);
        }
        else if (message.getPayload() instanceof MediaFormatNotFoundInFileException) {
            Log.e(TAG, "acceptMessage: " + message.getTitle());
            onEditDone(false);
            MediaFormatNotFoundInFileException e = (MediaFormatNotFoundInFileException) message.getPayload();
            e.printStackTrace();
            errorDialog(message.getTitle(), e.getMessage());
        }
        else if (message.getPayload() instanceof NullPointerException) {
            Log.e(TAG, "acceptMessage: " + message.getTitle());
            onEditDone(false);
            NullPointerException e = (NullPointerException) message.getPayload();
            e.printStackTrace();
            errorDialog(message.getTitle(), "Unable to load codec for this file");
        }
        else if (message.getPayload() instanceof MediaCodec.CodecException) {
            Log.e(TAG, "acceptMessage: " + message.getTitle());
            onEditDone(false);
            MediaCodec.CodecException e = (MediaCodec.CodecException) message.getPayload();
            e.printStackTrace();
            errorDialog(message.getTitle(), "Codec error: " + e.getErrorCode());
        }
    }

    public void onEditDone(boolean success) {
        this.runOnUiThread(() -> {
            // Hide the overlay
            animateView(progressOverlayView, View.GONE, 0, 200);
            resetEffects();
            resetTextOverlay();

            if (p != null) {
                Log.d(TAG, "onEditDone, stop contentLoader");
                p.stop();
                p = null;
            }

            if (success) {
                recreate();
                Toast.makeText(this, "Video export completed", Toast.LENGTH_SHORT).show();
            }
        });
    }

    public void animateView(final View view, final int toVisibility, float toAlpha, int duration) {
        boolean show = toVisibility == View.VISIBLE;
        if (show) {
            view.setAlpha(0);
        }
        view.setVisibility(View.VISIBLE);
        view.animate()
                .setDuration(duration)
                .alpha(show ? toAlpha : 0)
                .setListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        view.setVisibility(toVisibility);
                    }
                });
    }

    private void applyTextCompositing() {
        Log.d(TAG, "applyTextCompositing()");

        if (inputPath == null || p == null) {
            return;
        }

        String text = simpleEditText.getText().toString();
        String color = (String) spinnerTextColor.getSelectedItem();
        float opacity = sliderOpacityValue.getValue();

        View selectedRadioButton = rgTextPosition.findViewById(rgTextPosition.getCheckedRadioButtonId());
        int index = rgTextPosition.indexOfChild(selectedRadioButton);
        Constants.TextPosition position = Constants.TextPosition.values()[index];

        Log.d(TAG, "Compositing:  text=" + text + "  color=" + color + "  opacity=" + opacity + "  position=" + position);


        // The text bitmap is created in Java
        Size resolution = ContentLoader.getResolution(this, inputUri);
        Bitmap bitmap = Bitmap.createBitmap(resolution.getWidth(), resolution.getHeight(), Bitmap.Config.ARGB_8888);
        bitmap.eraseColor(0);
        Canvas canvas = new Canvas(bitmap);
        Paint textPaint = new Paint();

        Rect r = new Rect();
        textPaint.getTextBounds(text, 0, text.length(), r);
        textPaint.setTextSize(resolution.getHeight() / 9);
        textPaint.setAntiAlias(true);
        textPaint.setTextAlign(Paint.Align.CENTER);
        switch (color) {
            case Constants.BLACK:
                textPaint.setARGB(0xff, 0x00, 0x00, 0x00);
                break;
            case Constants.WHITE:
                textPaint.setARGB(0xff, 0xff, 0xff, 0xff);
                break;
            case Constants.GRAY:
                textPaint.setARGB(0x80, 0x80, 0x80, 0x00);
                break;
            case Constants.PURPLE:
                textPaint.setARGB(0xff, 0x62, 0x00, 0xee);
                break;
        }
        textPaint.setAlpha((int)(255 * opacity));

        int mStartX = (canvas.getWidth() / 2);
        int mStartY = (Math.abs(r.height())) / 2;
        int offset = canvas.getHeight() / 6;
        switch (position) {
            case TOP:
                mStartY += offset;
                break;
            case MIDDLE:
                mStartY += (offset * 3);
                break;
            case BOTTOM:
                mStartY += (offset * 5);
                break;
        }

        // Render text to the bitmap
        canvas.drawText(text, mStartX, mStartY, textPaint);

        // Convert bitmap to byte array
        int num_bytes = bitmap.getRowBytes() * bitmap.getHeight();
        ByteBuffer byteBuffer = ByteBuffer.allocate(num_bytes);
        bitmap.copyPixelsToBuffer(byteBuffer);
        byte[] byteArray = byteBuffer.array();

        // Send byte array to C++ shader
        p.EditShadersSetCompositingImage(byteArray);
        p.EditShadersSetParameter(EffectParameters.COMPOSITOR_ENABLE.ordinal(), 1.0f);
    }

    /*
    This is an example text overlay for testing.
    Call this method instead of applyTextCompositing() to apply the example.
     */
    public void setTestText() {
        Size videoDimensions = ContentLoader.getResolution(this, inputUri);
        int startX = videoDimensions.getWidth() / 9;
        int startY = videoDimensions.getHeight() / 3;
        int height = videoDimensions.getHeight() / 5;
        int offset1 = height / 20;
        int offset2 = height / 14;

        Bitmap bitmap = Bitmap.createBitmap(videoDimensions.getWidth(), videoDimensions.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        Paint textPaint = new Paint();
        textPaint.setTextSize(height);
        textPaint.setAntiAlias(true);

        textPaint.setARGB(0xff, 0, 0, 0);
        canvas.drawText("¡Yellow Whirled!", startX + offset1, startY + offset1, textPaint);
        textPaint.setARGB(0xff, 0xff, 0xff, 0);
        canvas.drawText("¡Yellow Whirled!", startX, startY, textPaint);

        textPaint.setARGB(0x5f, 0xff, 0, 0);
        canvas.drawText("Semi-transparent", startX, startY + height, textPaint);

        textPaint.setARGB(0x4f, 0, 0, 0);
        canvas.drawText("Drop Shadow", startX + offset2, startY + height * 2 + offset2, textPaint);
        textPaint.setARGB(0xff, 0, 0, 0xff);
        canvas.drawText("Drop Shadow", startX, startY + height * 2 , textPaint);

        int num_bytes = bitmap.getRowBytes() * bitmap.getHeight();
        ByteBuffer byteBuffer = ByteBuffer.allocate(num_bytes);
        bitmap.copyPixelsToBuffer(byteBuffer);
        byte[] byteArray = byteBuffer.array();

        p.EditShadersSetCompositingImage(byteArray);
        p.EditShadersSetParameter(EffectParameters.COMPOSITOR_ENABLE.ordinal(), 1.0f);
    }

}
