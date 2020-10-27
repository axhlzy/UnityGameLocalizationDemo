package com.lzy.dobbytest;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.io.Reader;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.UUID;

import static android.content.Context.TELEPHONY_SERVICE;

public class DeviceUtil {

    private static final String TAG = "DeviceUtil";
    private static final String UUID_FILE = ".phoneu.corefish";


    public static String getDeviceId(Context context) {
        try {
            String readIMEIFromExternalDir = readIMEIFromExternalDir(context);
            if (TextUtils.isEmpty(readIMEIFromExternalDir) || readIMEIFromExternalDir.equals("unknown")) {
                String imeiOrMeid = getImeiOrMeid(context);
                if (TextUtils.isEmpty(imeiOrMeid) || imeiOrMeid.equals("unknown")) {
                    String androidId = getAndroidId(context);
                    if (!TextUtils.isEmpty(androidId)) {
                        Log.i(TAG, "android.id - uuid=" + androidId);
                        return androidId;
                    }
                    String mac = getMac(context);
                    if (TextUtils.isEmpty(mac) || mac.equals("unknown")) {
                        String uUIDFromExternalDir = getUUIDFromExternalDir(context);
                        if (TextUtils.isEmpty(uUIDFromExternalDir)) {
                            return "000000000000000";
                        }
                        Log.i(TAG, "external.dir - uuid=" + uUIDFromExternalDir);
                        return uUIDFromExternalDir;
                    }
                    String replace = mac.replace(":", "");
                    Log.i(TAG, "mac.addr - uuid=" + replace);
                    return replace;
                }
                Log.i(TAG, "imei.meid - uuid=" + imeiOrMeid);
                return imeiOrMeid;
            }
            Log.i(TAG, "read.sdfile - uuid=" + readIMEIFromExternalDir);
            return readIMEIFromExternalDir;
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
            Log.i(TAG, "end - uuid=" + "000000000000000");
            return "000000000000000";
        }
    }

    private static String getUUIDFromExternalDir(Context context) {
        if (!checkPermission(context, "android.permission.READ_EXTERNAL_STORAGE")) {
            return "";
        }
        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/" + UUID_FILE);
        if (file.exists()) {
            try {
                BufferedReader bufferedReader = new BufferedReader(new FileReader(file));
                String readLine = bufferedReader.readLine();
                Log.i(TAG, "uuidCached=" + readLine);
                bufferedReader.close();
                if (!TextUtils.isEmpty(readLine)) {
                    return readLine;
                }
            } catch (IOException e) {
                e.printStackTrace();
                return "";
            }
        }
        try {
            if (!checkPermission(context, "android.permission.WRITE_EXTERNAL_STORAGE")) {
                return "";
            }
            if (!file.exists() && !file.createNewFile()) {
                return "";
            }
            String uuid = UUID.randomUUID().toString();
            Log.i(TAG, "uuidCreated=" + uuid);
            BufferedWriter bufferedWriter = new BufferedWriter(new FileWriter(file));
            String replace = uuid.replace("-", "");
            bufferedWriter.write(replace, 0, replace.length());
            bufferedWriter.flush();
            bufferedWriter.close();
            return replace;
        } catch (IOException e2) {
            e2.printStackTrace();
            return "";
        }
    }


    public static String readIMEIFromExternalDir(Context context) {
        if (!checkPermission(context, "android.permission.READ_EXTERNAL_STORAGE")) {
            return "";
        }
        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/" + UUID_FILE);
        StringBuilder sb = new StringBuilder();
        sb.append("writeIMEIToExternalDir uuidFile=");
        sb.append(file.getPath());
        Log.i(TAG, sb.toString());
        if (!file.exists()) {
            return "";
        }
        try {
            BufferedReader bufferedReader = new BufferedReader(new FileReader(file));
            String readLine = bufferedReader.readLine();
            Log.i(TAG, "uuidCached=" + readLine);
            bufferedReader.close();
            if (!TextUtils.isEmpty(readLine)) {
                return readLine;
            }
            return "";
        } catch (IOException e) {
            e.printStackTrace();
            return "";
        }
    }

    public static boolean checkPermission(Context context, String str) {
        if (context == null) {
            return false;
        }
        try {
            if (Build.VERSION.SDK_INT >= 23) {
                if (((Integer) Class.forName("android.content.Context").getMethod("checkSelfPermission", new Class[]{String.class}).invoke(context, new Object[]{str})).intValue() == 0) {
                    return true;
                }
            } else if (context.getPackageManager().checkPermission(str, context.getPackageName()) == PackageManager.PERMISSION_GRANTED) {
                return true;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    public static String getImeiOrMeid(Context context) {
        if (!checkPermission(context, "android.permission.READ_PHONE_STATE")) {
            return "";
        }
        TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(TELEPHONY_SERVICE);
        if (Build.VERSION.SDK_INT >= 26) {
            String imei = telephonyManager.getImei(0);
            if (!TextUtils.isEmpty(imei)) {
                Log.i(TAG, "imei0=" + imei);
                return imei;
            }
            String imei2 = telephonyManager.getImei(1);
            if (!TextUtils.isEmpty(imei2)) {
                Log.i(TAG, "imei1=" + imei2);
                return imei2;
            }
            String meid = telephonyManager.getMeid();
            if (!TextUtils.isEmpty(meid)) {
                Log.i(TAG, "meid=" + meid);
                return meid;
            }
            String serial = Build.getSerial();
            if (TextUtils.isEmpty(serial)) {
                return "";
            }
            Log.i(TAG, "serial=" + serial);
            return serial;
        } else if (Build.VERSION.SDK_INT >= 23) {
            String deviceId = telephonyManager.getDeviceId(0);
            if (!TextUtils.isEmpty(deviceId)) {
                Log.i(TAG, "deviceId0=" + deviceId);
                return deviceId;
            }
            String deviceId2 = telephonyManager.getDeviceId(1);
            if (TextUtils.isEmpty(deviceId2)) {
                return "";
            }
            Log.i(TAG, "deviceId1=" + deviceId2);
            return deviceId2;
        } else {
            String deviceId3 = telephonyManager.getDeviceId();
            if (TextUtils.isEmpty(deviceId3)) {
                return "";
            }
            Log.i(TAG, "deviceId - uuid=" + deviceId3);
            return deviceId3;
        }
    }

    private static String getAndroidId(Context context) {
        return Settings.Secure.getString(context.getContentResolver(), "android_id");
    }

    private static String getMac(Context context) {
        if (context == null) {
            return "";
        }
        if (Build.VERSION.SDK_INT < 23) {
            String macBySystemInterface = getMacBySystemInterface(context);
            if (TextUtils.isEmpty(macBySystemInterface)) {
                return "";
            }
            Log.i(TAG, "uuid using sysinter");
            return macBySystemInterface;
        } else if (Build.VERSION.SDK_INT < 24) {
            String mac0 = getMac0();
            if (TextUtils.isEmpty(mac0)) {
                return "";
            }
            Log.i(TAG, "uuid using wlan");
            return mac0;
        } else {
            String machineHardwareAddress = getMachineHardwareAddress();
            if (!TextUtils.isEmpty(machineHardwareAddress)) {
                Log.i(TAG, "uuid using hardaddr");
                return machineHardwareAddress;
            }
            String localMacAddressFromBusybox = getLocalMacAddressFromBusybox();
            if (!TextUtils.isEmpty(localMacAddressFromBusybox)) {
                Log.i(TAG, "uuid using busybox");
                return localMacAddressFromBusybox;
            }
            String macAddress = getMacAddress();
            if (TextUtils.isEmpty(macAddress) || "02:00:00:00:00:00".equals(macAddress)) {
                String macBySystemInterface2 = getMacBySystemInterface(context);
                if (TextUtils.isEmpty(macBySystemInterface2)) {
                    return "";
                }
                Log.i(TAG, "uuid using sysinter");
                return macBySystemInterface2;
            }
            Log.i(TAG, "uuid using macaddr");
            return macAddress;
        }
    }

    private static String getMac0() {
        String str = "";
        String str2 = "";
        try {
            LineNumberReader lineNumberReader = new LineNumberReader(new InputStreamReader(Runtime.getRuntime().exec("cat /sys/class/net/wlan0/address").getInputStream()));
            while (true) {
                if (str != null) {
                    str = lineNumberReader.readLine();
                    if (str != null) {
                        str2 = str.trim();
                        break;
                    }
                } else {
                    break;
                }
            }
            if (!TextUtils.isEmpty(str2)) {
                return str2;
            }
            String substring = loadFileAsString("/sys/class/net/eth0/address").toUpperCase().substring(0, 17);
            if (!TextUtils.isEmpty(substring)) {
                return substring;
            }
            return "";
        } catch (Exception e) {
            Log.e(TAG, "getMacAddress:" + e.toString());
            return "";
        }
    }

    private static String loadFileAsString(String str) throws Exception {
        FileReader fileReader = new FileReader(str);
        String loadReaderAsString = loadReaderAsString(fileReader);
        fileReader.close();
        return loadReaderAsString;
    }

    private static String loadReaderAsString(Reader reader) throws Exception {
        StringBuilder sb = new StringBuilder();
        char[] cArr = new char[4096];
        int read = reader.read(cArr);
        while (read >= 0) {
            sb.append(cArr, 0, read);
            read = reader.read(cArr);
        }
        return sb.toString();
    }

    @SuppressLint("HardwareIds")
    public static String getMacAddress(Application app) {
        return ((WifiManager) app.getSystemService(Context.WIFI_SERVICE)).getConnectionInfo().getMacAddress();
    }

    private static int getNetType(ConnectivityManager connectivityManager) {
        NetworkInfo activeNetworkInfo;
        if (connectivityManager == null || (activeNetworkInfo = connectivityManager.getActiveNetworkInfo()) == null) {
            return 0;
        }
        switch (activeNetworkInfo.getType()) {
            case 0:
                return 1;
            case 1:
                return 2;
            default:
                return 0;
        }
    }


    public static String getMacAddress() {
        try {
            byte[] hardwareAddress = NetworkInterface.getByInetAddress(getLocalInetAddress()).getHardwareAddress();
            StringBuffer stringBuffer = new StringBuffer();
            for (int i = 0; i < hardwareAddress.length; i++) {
                if (i != 0) {
                    stringBuffer.append(":");
                }
                String hexString = Integer.toHexString(hardwareAddress[i] & 255);
                if (hexString.length() == 1) {
                    hexString = 0 + hexString;
                }
                stringBuffer.append(hexString);
            }
            return stringBuffer.toString().toUpperCase();
        } catch (Exception e) {
            e.printStackTrace();
            return "";
        }
    }

    private static InetAddress getLocalInetAddress() {
        InetAddress inetAddress;
        SocketException e;
        try {
            Enumeration<NetworkInterface> networkInterfaces = NetworkInterface.getNetworkInterfaces();
            inetAddress = null;
            do {
                if (!networkInterfaces.hasMoreElements()) {
                    break;
                }
                Enumeration<InetAddress> inetAddresses = networkInterfaces.nextElement().getInetAddresses();
                while (true) {
                    if (!inetAddresses.hasMoreElements()) {
                        break;
                    }
                    InetAddress nextElement = inetAddresses.nextElement();
                    if (!nextElement.isLoopbackAddress() && nextElement.getHostAddress().indexOf(":") == -1) {
                        inetAddress = nextElement;
                        continue;
                    }
                    inetAddress = null;
                }
            } while (inetAddress == null);
        } catch (SocketException e4) {
            inetAddress = null;
            e = e4;
            e.printStackTrace();
            return inetAddress;
        }
        return inetAddress;
    }



    private static String getMacBySystemInterface(Context context) {
        if (context == null) {
            return "";
        }
        try {
            WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (checkPermission(context, "android.permission.ACCESS_WIFI_STATE")) {
                return wifiManager.getConnectionInfo().getMacAddress();
            }
            return "";
        } catch (Throwable th) {
            th.printStackTrace();
            return "";
        }
    }

    public static String getMachineHardwareAddress() {
        try {
            Enumeration<NetworkInterface> networkInterfaces = NetworkInterface.getNetworkInterfaces();
            String str = null;
            if (networkInterfaces == null) {
                return null;
            }
            while (networkInterfaces.hasMoreElements()) {
                try {
                    String bytesToString = bytesToString(networkInterfaces.nextElement().getHardwareAddress());
                    if (bytesToString != null) {
                        return bytesToString;
                    }
                    str = bytesToString;
                } catch (SocketException e) {
                    e.printStackTrace();
                }
            }
            return str;
        } catch (SocketException e2) {
            e2.printStackTrace();
            return "";
        }
    }

    public static String getLocalMacAddressFromBusybox() {
        String callCmd = callCmd("busybox ifconfig", "HWaddr");
        if (callCmd == null) {
            return "";
        }
        if (callCmd.length() <= 0 || !callCmd.contains("HWaddr")) {
            return callCmd;
        }
        return callCmd.substring(callCmd.indexOf("HWaddr") + 6, callCmd.length() - 1);
    }

    private static String callCmd(String str, String str2) {
        String str3 = "";
        try {
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(Runtime.getRuntime().exec(str).getInputStream()));
            while (true) {
                String readLine = bufferedReader.readLine();
                if (readLine == null || readLine.contains(str2)) {
                    return readLine;
                }
                str3 = str3 + readLine;
            }
        } catch (Exception e) {
            String str4 = str3;
            e.printStackTrace();
            return str4;
        }
    }

    private static String bytesToString(byte[] bArr) {
        if (bArr == null || bArr.length == 0) {
            return null;
        }
        StringBuilder sb = new StringBuilder();
        int length = bArr.length;
        for (int i = 0; i < length; i++) {
            sb.append(String.format("%02X:", new Object[]{Byte.valueOf(bArr[i])}));
        }
        if (sb.length() > 0) {
            sb.deleteCharAt(sb.length() - 1);
        }
        return sb.toString();
    }



}
