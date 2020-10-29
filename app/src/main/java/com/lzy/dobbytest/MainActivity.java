package com.lzy.dobbytest;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.security.NoSuchAlgorithmException;

import static com.lzy.dobbytest.MD5Utils.md5;

public class MainActivity extends AppCompatActivity {

    long lastTime = System.currentTimeMillis();
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private TextView tv;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tv = findViewById(R.id.sample_text);
        tv.setText(DeviceUtil.getDeviceId(this));

        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    md5(stringFromJNI().getBytes());
                } catch (NoSuchAlgorithmException e) {
                    e.printStackTrace();
                }
                tv.setText(stringFromJNI()+"\ncurrent:"+System.currentTimeMillis());
            }
        });
    }

    public native String stringFromJNI();

}
