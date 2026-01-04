package com.picow.lennyface;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.Toast;
import android.content.ClipboardManager;
import android.content.ClipData;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class MainActivity extends Activity {

    private static final String ACTION_USB_PERMISSION = "com.picow.lennyface.USB_PERMISSION";

    private UsbManager usbManager;
    private UsbDevice device;
    private UsbDeviceConnection connection;
    private UsbInterface cdcInterface;
    private UsbEndpoint readEndpoint;
    private UsbEndpoint writeEndpoint;

    private TextView statusText;
    private ClipboardManager clipboard;

    private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if (device != null) {
                            setupDevice(device);
                        }
                    } else {
                        statusText.setText("USB permission denied");
                    }
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Simple UI
        statusText = new TextView(this);
        statusText.setText("Lenny Face Keyboard\nWaiting for Pico W...");
        statusText.setTextSize(18);
        statusText.setPadding(50, 50, 50, 50);
        setContentView(statusText);

        clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
        usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        // Register USB permission receiver
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        registerReceiver(usbReceiver, filter);

        // Check for attached devices
        checkForDevice();
    }

    @Override
    protected void onDestroy() {
        unregisterReceiver(usbReceiver);
        closeDevice();
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        super.onResume();
        checkForDevice();
    }

    private void checkForDevice() {
        // Get list of attached devices
        HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();

        // Look for our device (VID=0xCafe, PID=0x4001)
        for (Map.Entry<String, UsbDevice> entry : deviceList.entrySet()) {
            UsbDevice dev = entry.getValue();
            if (dev.getVendorId() == 0xCafe && dev.getProductId() == 0x4001) {
                requestDevicePermission(dev);
                return;
            }
        }

        statusText.setText("Lenny Face Keyboard\n\nConnect Pico W via USB OTG\nShort GP4 and GP5 to send lenny face");
    }

    private void requestDevicePermission(UsbDevice device) {
        if (usbManager.hasPermission(device)) {
            setupDevice(device);
        } else {
            PendingIntent permissionIntent = PendingIntent.getBroadcast(
                    this, 0, new Intent(ACTION_USB_PERMISSION), PendingIntent.FLAG_IMMUTABLE);
            usbManager.requestPermission(device, permissionIntent);
        }
    }

    private void setupDevice(UsbDevice device) {
        this.device = device;
        closeDevice(); // Close any existing connection

        // Find CDC interface (usually interface 0 or 1)
        // For composite devices, CDC is typically the first interface
        UsbInterface cdcInterface = null;
        UsbEndpoint readEndpoint = null;
        UsbEndpoint writeEndpoint = null;

        for (int i = 0; i < device.getInterfaceCount(); i++) {
            UsbInterface iface = device.getInterface(i);
            int endpointCount = iface.getEndpointCount();

            for (int j = 0; j < endpointCount; j++) {
                UsbEndpoint endpoint = iface.getEndpoint(j);

                // CDC endpoints
                if (endpoint.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                    if (endpoint.getDirection() == UsbConstants.USB_DIR_IN) {
                        readEndpoint = endpoint;
                    } else if (endpoint.getDirection() == UsbConstants.USB_DIR_OUT) {
                        writeEndpoint = endpoint;
                    }
                }
            }

            if (readEndpoint != null && writeEndpoint != null) {
                cdcInterface = iface;
                break;
            }
        }

        if (cdcInterface == null || readEndpoint == null) {
            statusText.setText("CDC interface not found");
            return;
        }

        this.cdcInterface = cdcInterface;
        this.readEndpoint = readEndpoint;
        this.writeEndpoint = writeEndpoint;

        // Open connection
        connection = usbManager.openDevice(device);
        if (connection == null) {
            statusText.setText("Failed to open USB device");
            return;
        }

        // Claim interface
        if (connection.claimInterface(cdcInterface, true)) {
            statusText.setText("Connected to Pico W\n\nShort GP4 and GP5\nto send lenny face");
            startReading();
        } else {
            statusText.setText("Failed to claim interface");
            closeDevice();
        }
    }

    private void startReading() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                byte[] buffer = new byte[256];
                while (connection != null && readEndpoint != null) {
                    int bytesRead = connection.bulkTransfer(readEndpoint, buffer, buffer.length, 100);

                    if (bytesRead > 0) {
                        final String data = new String(buffer, 0, bytesRead, StandardCharsets.UTF_8);
                        handleReceivedData(data);
                    }
                }
            }
        }).start();
    }

    private void handleReceivedData(final String data) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Copy received text to clipboard
                ClipData clip = ClipData.newPlainText("Lenny Face", data);
                clipboard.setPrimaryClip(clip);

                statusText.setText("Copied to clipboard:\n" + data + "\n\nPaste with Ctrl+V or long-press");

                Toast.makeText(MainActivity.this, "Copied: " + data, Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void closeDevice() {
        if (connection != null && cdcInterface != null) {
            connection.releaseInterface(cdcInterface);
            connection.close();
        }
        connection = null;
        cdcInterface = null;
        readEndpoint = null;
        writeEndpoint = null;
    }
}
