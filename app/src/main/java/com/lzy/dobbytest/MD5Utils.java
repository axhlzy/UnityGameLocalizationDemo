package com.lzy.dobbytest;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class MD5Utils {

    public static final String SHARK = "SHARK";

    public static void main(String[] args) throws NoSuchAlgorithmException, NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        String md5 = md5("Hello from C++".getBytes());
        System.out.println(md5);
        System.out.println(md5.substring(8,24));

        Class<?>[] interfaces = MainActivity.class.getInterfaces();
        Method[] declaredMethods = MainActivity.class.getDeclaredMethods();

        for (int i = 0; i < interfaces.length; i++) {
            System.out.println(interfaces[i]);
        }

        for (int i = 0; i < declaredMethods.length; i++) {
            System.out.println(declaredMethods[i]);
        }
    }

    public static String md5(byte[] input) throws NoSuchAlgorithmException {
        String md5 = "";
        MessageDigest messageDigest = MessageDigest.getInstance("SHA");
        byte[] buf = messageDigest.digest(input);
        for (byte b : buf){
            int val = b;
            if(val < 0){
                val += 256;
            }
            String str = "" + Integer.toHexString(val);
            if(str.length() == 1){
                str = "0" + str;
            }
            md5 += str;
        }
        boolean contains = new String().contains("2");
        return md5;
    }

    public void showLog(){
        System.out.println("this is a string showLog! ");
    }

    public void showLog(int a){
        System.out.println("this is a string showLog! "+a);
    }

    public void showLog(String a){
        System.out.println("this is a string showLog! "+a + "String");
    }

    public void showLog(char a){
        System.out.println("this is a string showLog! "+a + "Int");
    }

    public void showLog(byte a){
        System.out.println("this is a string showLog! "+a + "Byte");
    }

    public void showLog(boolean a){
        System.out.println("this is a string showLog! "+a + "33333");
    }

    public void showLog(boolean a,int  b){
        System.out.println("this is a string showLog! "+a + b + "boolean int");
    }

    void getSignature(Context ctx) {
        try {
            PackageInfo packageInfo = ctx.getPackageManager().getPackageInfo(ctx.getPackageName(), PackageManager.GET_SIGNATURES);
            Log.i(SHARK, "len:"+packageInfo.signatures.length);
            if (packageInfo.signatures != null) {
                Log.i(SHARK, "sig:"+packageInfo.signatures[0].toCharsString());
            }
        } catch (Exception e) {
        }
    }
}