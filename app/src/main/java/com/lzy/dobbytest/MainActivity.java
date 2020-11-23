package com.lzy.dobbytest;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.TextView;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.security.NoSuchAlgorithmException;

import static com.lzy.dobbytest.MD5Utils.md5;

public class MainActivity extends AppCompatActivity {

    long lastTime = System.currentTimeMillis();
    String a = "\"{\"placement_reward_name\":\"Virtual Item\",\"placement_name\":\"DefaultRewardedVideo\",\"placement_reward_amount\":\"1\",\"placement_id\":\"1\"}\"";
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
        tv.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                return false;
            }
        });

        try {
            Method showLog0 = MD5Utils.class.getDeclaredMethod("showLog");
            Method showLog1 = MD5Utils.class.getDeclaredMethod("showLog", String.class);
            Method showLog2 = MD5Utils.class.getDeclaredMethod("showLog", int.class);
            showLog0.invoke(new MD5Utils());
            showLog1.invoke(new MD5Utils(),"abc");
            showLog2.invoke(new MD5Utils(),23123);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            e.printStackTrace();
        }

        new MD5Utils().getSignature(this);

    }

    public native String stringFromJNI();

    @Override
    protected void attachBaseContext(Context newBase) {
        super.attachBaseContext(newBase);
    }
}
