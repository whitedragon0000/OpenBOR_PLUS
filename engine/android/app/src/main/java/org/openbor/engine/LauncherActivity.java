package org.openbor.engine; // Make sure this matches your package name

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

// You might need to import AlertDialog and DialogInterface if you use showErrorAndRetryDialog
// import android.app.AlertDialog;
// import android.content.DialogInterface;

public class LauncherActivity extends Activity {

    private static final String TAG = "LauncherActivity";
    private static final int PICK_PAK_FILE_REQUEST_CODE = 1;
    private static final String DEST_SUB_FOLDER_NAME = "Paks"; // Subdirectory for copied .pak files

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Set your layout if you have one, or keep it simple for testing
        // setContentView(R.layout.activity_launcher);

        // Immediately try to show the file picker when the activity is created
        showPakSelectionDialog();
    }

    private void showPakSelectionDialog() {
        Log.d(TAG, "Showing .pak file selection dialog...");
        Toast.makeText(this, "Select .pak file to copy!.", Toast.LENGTH_LONG).show();
        // ACTION_OPEN_DOCUMENT allows the user to pick a document that is "owned" by an app.
        // This is suitable for files the user wants to grant you access to, like from Downloads.
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);

        // Filter to show only files that can be opened.
        intent.addCategory(Intent.CATEGORY_OPENABLE);

        // Specify the MIME type for .pak files or general binary files.
        // A specific MIME type like "application/octet-stream" is often used for arbitrary binary data.
        // If your .pak files have a known MIME type, use that.
        intent.setType("*/*"); // Allow all file types for broader selection
        String[] mimeTypes = {"application/octet-stream", "application/zip", "application/x-pak"}; // Common for custom data, zip
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);


        // OPTIONAL: Suggest a starting location (Android 11+ for this to be consistently respected)
        // This tries to open the picker directly in the Downloads directory.
        // It's a hint and might not work on all devices/Android versions.
        try {
            Uri downloadsUri = Uri.parse("content://com.android.providers.downloads.documents/document/downloads");
            if (downloadsUri != null) {
                 intent.putExtra(android.provider.DocumentsContract.EXTRA_INITIAL_URI, downloadsUri);
            }
        } catch (Exception e) {
            Log.w(TAG, "Could not set initial URI to Downloads. " + e.getMessage());
        }


        try {
            startActivityForResult(intent, PICK_PAK_FILE_REQUEST_CODE);
        } catch (Exception e) {
            Log.e(TAG, "Could not launch file picker: " + e.getMessage(), e);
            Toast.makeText(this, "Error opening file picker. Please ensure a file manager is installed.", Toast.LENGTH_LONG).show();
            // Decide what to do if the picker cannot be launched (e.g., finish activity or retry)
            finish(); // For this simple example, we'll just exit
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == PICK_PAK_FILE_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK && data != null && data.getData() != null) {
                Uri uri = data.getData();
                Log.d(TAG, "Selected file URI: " + uri.toString());

                String fileName = getFileName(uri); // Helper method to get file name from URI

                if (fileName != null && fileName.toLowerCase().endsWith(".pak")) {
                    File destinationRootFolder = new File(getExternalFilesDir(null), DEST_SUB_FOLDER_NAME);
                    // Ensure the destination folder exists
                    if (!destinationRootFolder.exists()) {
                        if (!destinationRootFolder.mkdirs()) {
                            Log.e(TAG, "Failed to create destination folder: " + destinationRootFolder.getAbsolutePath());
                            Toast.makeText(this, "Failed to create game data folder.", Toast.LENGTH_LONG).show();
                            showErrorAndRetryDialog("Failed to prepare game data folder. Try again?");
                            return;
                        }
                    }

                    File destinationFile = new File(destinationRootFolder, fileName);

                    // Check if the file already exists in the destination
                    if (destinationFile.exists()) {
                        Log.d(TAG, "File " + fileName + " already exists in Paks folder. Launching game.");
                        Toast.makeText(this, fileName + " already in game folder. Starting game.", Toast.LENGTH_LONG).show();
                        startGameActivity();
                    } else {
                        Log.d(TAG, "Selected file: " + fileName + " is a .pak file and needs to be copied. Copying...");
                        copySelectedPakFile(uri, fileName);
                    }
                } else {
                    Log.w(TAG, "Selected file is not a .pak file: " + (fileName != null ? fileName : "Unknown"));
                    Toast.makeText(this, "Please select a file with the .pak extension.", Toast.LENGTH_LONG).show();
                    showPakSelectionDialog(); // Prompt user again
                }
            } else if (resultCode == Activity.RESULT_CANCELED) {
                Log.d(TAG, "File selection cancelled by user.");
                Toast.makeText(this, "File selection cancelled.", Toast.LENGTH_SHORT).show();
                // If user cancels, we might want to prompt again or exit
                startGameActivity(); // Continue to start engine
            } else {
                Log.e(TAG, "Unknown result from file picker: resultCode=" + resultCode);
                Toast.makeText(this, "An error occurred during file selection.", Toast.LENGTH_LONG).show();
                showPakSelectionDialog(); // Re-prompt
            }
        }
    }

    private void copySelectedPakFile(Uri pakFileUri, String fileName) {
        File destinationRootFolder = new File(getExternalFilesDir(null), DEST_SUB_FOLDER_NAME);
        File destinationFile = new File(destinationRootFolder, fileName);

        InputStream in = null;
        OutputStream out = null;
        try {
            // !!! Crucial for ACTION_OPEN_DOCUMENT URIs !!!
            // Grants your app persistent read access to the selected URI.
            // This is the direct solution to the SecurityException you encountered earlier.
            final int takeFlags = Intent.FLAG_GRANT_READ_URI_PERMISSION;
            getContentResolver().takePersistableUriPermission(pakFileUri, takeFlags);

            in = getContentResolver().openInputStream(pakFileUri);
            if (in == null) {
                throw new IOException("Failed to open input stream for URI: " + pakFileUri.toString());
            }
            out = new FileOutputStream(destinationFile);
            copyFile(in, out); // Your utility copy method
            Toast.makeText(this, "Copied " + fileName + " to game data folder!", Toast.LENGTH_LONG).show();
            Log.d(TAG, "Successfully copied: " + fileName + " to " + destinationFile.getAbsolutePath());
            startGameActivity(); // Launch the game after successful copy
        } catch (IOException e) {
            Log.e(TAG, "Failed to copy selected .pak file: " + fileName, e);
            Toast.makeText(this, "Error copying " + fileName + ". Please try again.", Toast.LENGTH_LONG).show();
            // Clean up partially copied file
            if (destinationFile.exists() && !destinationFile.delete()) {
                Log.w(TAG, "Could not delete partially copied file: " + destinationFile.getAbsolutePath());
            }
            showErrorAndRetryDialog("Failed to copy " + fileName + ". Would you like to try again?");
        } catch (SecurityException e) {
            Log.e(TAG, "SecurityException while copying file: " + e.getMessage(), e);
            Toast.makeText(this, "Permission denied to read selected file. Please ensure app has storage access if prompted, or choose an accessible file.", Toast.LENGTH_LONG).show();
            showErrorAndRetryDialog("Permission denied. Select another file?");
        } finally {
            try {
                if (in != null) in.close();
                if (out != null) {
                    out.flush(); // Ensure all buffered data is written
                    out.close();
                }
            } catch (IOException e) {
                Log.e(TAG, "Error closing streams for " + fileName + ": " + e.getMessage());
            }
        }
    }

    // --- Utility Methods (Keep these as they were or adapt them) ---

    // This method needs to be implemented based on your actual game launch
private void startGameActivity() {
    Log.d(TAG, "Attempting to start GameActivity..."); // <--- ADD THIS
    Toast.makeText(this, "Starting OpenBOR!", Toast.LENGTH_SHORT).show();
    Intent gameIntent = new Intent(this, GameActivity.class); // Assuming GameActivity is your game's main activity
    startActivity(gameIntent);
    finish(); // Optional: close LauncherActivity if it's no longer needed
}

    // This is a placeholder; implement your actual dialog logic
    private void showErrorAndRetryDialog(String message) {
        Log.e(TAG, "Error: " + message);
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
        // Example of a simple dialog, you'll need AlertDialog imports:
        // new AlertDialog.Builder(this)
        //     .setTitle("Error")
        //     .setMessage(message)
        //     .setPositiveButton("Retry", new DialogInterface.OnClickListener() {
        //         public void onClick(DialogInterface dialog, int which) {
        //             showPakSelectionDialog(); // Try again
        //         }
        //     })
        //     .setNegativeButton("Exit", new DialogInterface.OnClickListener() {
        //         public void onClick(DialogInterface dialog, int which) {
        //             finish(); // Exit the app
        //         }
        //     })
        //     .setCancelable(false) // User must choose an option
        //     .show();
        showPakSelectionDialog(); // For this example, just re-prompt directly
    }

    // Helper method to get the file name from a content URI
    // This is a common pattern for ACTION_OPEN_DOCUMENT URIs.
    private String getFileName(Uri uri) {
        String result = null;
        if (uri.getScheme().equals("content")) {
            try (android.database.Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                    if (nameIndex != -1) {
                        result = cursor.getString(nameIndex);
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Error getting file name from URI: " + uri.toString(), e);
            }
        }
        if (result == null) {
            result = uri.getLastPathSegment(); // Fallback
        }
        return result;
    }

    // Utility method to copy an InputStream to an OutputStream
    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }
}