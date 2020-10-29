package com.lzy.dobbytest;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class MD5Utils {

    public static void main(String[] args) throws NoSuchAlgorithmException {
        String md5 = md5("Hello from C++".getBytes());
        System.out.println(md5);
        System.out.println(md5.substring(8,24));

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
        return md5;
    }
}