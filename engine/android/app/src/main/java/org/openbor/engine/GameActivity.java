/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 *
 * Moved from SDLActivity.java here for more flexibility.
 * IMPORTANT: DON'T EDIT SDLActivity.java anymore, but this file!
 *
 * The following from SDLActivity.java migration, and kept intact for respect to authors
 * as well as specific lines inside this source file is kept intact although moved / rearranged /
 * removed / modified as part from migration process.
 * --------------------------------------------------------
 * SDLActivity.java - Main code for Android build.
 * Original by UTunnels (utunnels@hotmail.com).
 * Modifications by CRxTRDude, White Dragon and msmalik681.
 * --------------------------------------------------------
 */

package org.openbor.engine;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.VibrationEffect;
import android.os.Vibrator;

import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.libsdl.app.SDLActivity;
import org.openbor.engine.utils.FrameDimensions;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

//msmalik681 added imports for new pak copy!
//msmalik681 added import for permission check

/**
 * Extended functionality from SDLActivity.
 *
 * Separated for ease of updating both for dependency and this support functionality later.
 */
public class GameActivity extends SDLActivity {
  /**
   * Needed for permission check
   */
  public static final int STORAGE_PERMISSION_CODE = 23;

  //White Dragon: added statics
  protected static WakeLock wakeLock;

  @SuppressWarnings("JavaJniMissingFunction")
  public static native void fireSystemUiVisibilityChangeEvent(int isSystemBarsVisible);

  //note: White Dragon's vibrator is moved into C code for 2 reasons
  // - avoid modifying SDLActivity.java as it's platform support
  // - reduce round-trip cost/time in call C-function to check for touch-area and whether
  //   vibration is enabled or not
  //   (for reference: SDL finally registers event/action/x/y/etc into its C-code from Java code
  //   in onTouch() call, thus we do this logic in C code for efficient then provide vibration code
  //   in Java when we really need to vibrate the device)

  // -- section of Java native solutions provided to be called from C code -- //
  /**
   * This will vibrate device if there's vibrator service.
   * Otherwise it will do nothing.
   *
   * Modified version from original by White Dragon
   */
  @SuppressWarnings("unused")
  public static void jni_vibrate(int intensity) {
    Vibrator vibrator = (Vibrator)getContext().getSystemService(Context.VIBRATOR_SERVICE);

      // wait for 3 ms, vibrate for 250 ms, then off for 1000 ms
      // note: consult api at two links below, it has two different meanings but in this case,
      // use case is the same
      long[] mVibratePattern  = new long[]{16, 250};

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
      {
        if (vibrator.hasVibrator()) {
          // calculate intensity ratio value
          int amp = (intensity * 255) / 100;
          int[] mAmplitudes = new int[]{0, amp};
          // API 26 and above
          // look for its api at https://developer.android.com/reference/android/os/VibrationEffect.html
          vibrator.vibrate(VibrationEffect.createWaveform(mVibratePattern, mAmplitudes, -1));
        }
      }
      else
      {
        // below API 26
        // look for its api at https://developer.android.com/reference/android/os/Vibrator.html#vibrate(long%5B%5D,%2520int)
        vibrator.vibrate(mVibratePattern, -1);
      }
  }

  @SuppressWarnings("unused")
  public static FrameDimensions jni_get_frame_dimensions() {
    // include navigation bar dimensions
    /*Resources resources = getContext().getResources();
    int resourceId = resources.getIdentifier("navigation_bar_height", "dimen", "android");
    int nav_bar_h = resources.getDimensionPixelSize(resourceId);
    if (resourceId > 0) {
        int orientation = resources.getConfiguration().orientation;
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            // In landscape
            frameDimensions.setRight(frameDimensions.getRight() + nav_bar_h);
        } else {
            // In portrait
            frameDimensions.setBottom(frameDimensions.getBottom() + nav_bar_h);
        }
    }*/
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
    {
      Activity activity = (Activity) getContext();

      Rect rectangle = new Rect();
      View view = activity.getWindow().getDecorView().getRootView();
      view.getWindowVisibleDisplayFrame(rectangle);
      return new FrameDimensions((int) view.getTranslationX(), (int) view.getTranslationY(), view.getWidth(), view.getHeight(),
              rectangle.top, rectangle.left, rectangle.bottom, rectangle.right);
    } else return null;
  }
  // ------------------------------------------------------------------------ //

  /**
   * Also load "openbor" as shared library to run the game in which
   * inside there's main function entry for the program.
   */
  @Override
  protected String[] getLibraries() {
    return new String[] {
      "SDL2",
      "openbor"
    };
  }

  @SuppressLint("InvalidWakeLockTag")
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    // call parent's implementation
    super.onCreate(savedInstanceState);
    Log.v("OpenBOR", "onCreate called");

    /*Activity activity = (Activity)getContext();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD
            && Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2) {
      activity.setRequestedOrientation(
              ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.GINGERBREAD) {
      activity.setRequestedOrientation(
              ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }*/

    //msmalik681 setup storage access
    CheckPermissionForMovingPaks();

    //CRxTRDude - Added FLAG_KEEP_SCREEN_ON to prevent screen timeout.
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    //CRxTRDude - Created a wakelock to prevent the app from being shut down upon screen lock.
    PowerManager pm = (PowerManager)getSystemService(POWER_SERVICE);
    GameActivity.wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "BOR");
    if (!GameActivity.wakeLock.isHeld())
    {
      GameActivity.wakeLock.acquire(10*60*1000L /*10 minutes*/);
    }

    View decorView = getWindow().getDecorView().getRootView();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
    {
      decorView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener()
      {
        @Override
        public void onSystemUiVisibilityChange(int visibility)
        {
          // Note that system bars will only be "visible" if none of the
          // LOW_PROFILE, HIDE_NAVIGATION, or FULLSCREEN flags are set.
          if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
          {
            // The system bars are visible.
            fireSystemUiVisibilityChangeEvent(1);
          }
          else
          {
            // The system bars are NOT visible
            fireSystemUiVisibilityChangeEvent(0);
          }
        }
      });

      decorView.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
      {
        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                   int oldLeft, int oldTop, int oldRight, int oldBottom)
        {
          //decorView.removeOnLayoutChangeListener(this);
          if (left != oldLeft || top != oldTop || right != oldRight || bottom != oldBottom)
          {
            fireSystemUiVisibilityChangeEvent(0);
          }
        }
      });
    }

  }

  //msmalik681 added permission check for API 23+ for moving .paks
  private void CheckPermissionForMovingPaks() {
    if (Build.VERSION.SDK_INT >= STORAGE_PERMISSION_CODE &&
        getApplicationContext().getPackageName().equals("org.openbor.engine"))
    {
      if (ContextCompat.checkSelfPermission(GameActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED &&
          ContextCompat.checkSelfPermission(GameActivity.this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
      {
        Toast.makeText(this, "Needed permissions not granted!", Toast.LENGTH_LONG).show();
        ActivityCompat.requestPermissions(GameActivity.this, new String[] {
          Manifest.permission.WRITE_EXTERNAL_STORAGE,
          Manifest.permission.READ_EXTERNAL_STORAGE
        }, STORAGE_PERMISSION_CODE);
      }
      else
      {
        CopyPak();
      }
    }
    else
    {
      CopyPak();
    }
  }

  /*@Override
  public void onConfigurationChanged(Configuration newConfig)
  {
    super.onConfigurationChanged(newConfig);

    if (newConfig.orientation != Configuration.ORIENTATION_LANDSCAPE)
    {
      Toast.makeText(this, "not landscape", Toast.LENGTH_SHORT).show();
      Activity activity = (Activity)getContext();
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD
              && Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2) {
        activity.setRequestedOrientation(
                ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
      } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.GINGERBREAD) {
        activity.setRequestedOrientation(
                ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
      }
    }
  }*/

  @SuppressWarnings("NullableProblems")
  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
  {
    if (requestCode == STORAGE_PERMISSION_CODE) {// If request is cancelled, the result arrays are empty.
      if (grantResults.length > 0 &&
              grantResults[0] == PackageManager.PERMISSION_GRANTED) {
        // permission was granted continue!
        CopyPak();
      } else {
        // needed permission denied end application!
        finish();
      }
    }
  }

  /**
   * Proceed in copying paks files, or just prepare the destination Paks directory depending
   * on which type of app it is.
   */
  @SuppressWarnings("ResultOfMethodCallIgnored")
  public void CopyPak()
  {
    try {
      Context ctx = getContext();
      Context appCtx = getApplicationContext();
      String toast;

      // if package name is literally "org.openbor.engine" then we have no need to copy any .pak files
      if (appCtx.getPackageName().equals("org.openbor.engine"))
      {
        // Default output folder
        File outFolderDefault = new File(Environment.getExternalStorageDirectory() + "/OpenBOR/Paks");

        if (!outFolderDefault.isDirectory())
        {
          outFolderDefault.mkdirs();
          toast = "Folder: (" + outFolderDefault + ") is empty!";
          Toast.makeText(appCtx, toast, Toast.LENGTH_LONG).show();
        }
        else
        {
          String[] files = outFolderDefault.list();
          if (files != null && files.length == 0)
          {
            // directory is empty
            toast = "Paks Folder: (" + outFolderDefault + ") is empty!";
            Toast.makeText(appCtx, toast, Toast.LENGTH_LONG).show();
          }
        }
      }
      // otherwise it acts like a dedicated app (commercial title, standalone app)
      // intend to work with pre-baked single .pak file at build time
      else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO)
      {
        // versionName is "android:versionName" in AndroidManifest.xml
        String version = appCtx.getPackageManager().getPackageInfo(appCtx.getPackageName(), 0).versionName;  // get version number as string
        // set local output folder (primary shared/external storage)
        File outFolder = new File(ctx.getExternalFilesDir(null) + "/Paks");
        // set local output filename as version number
        File outFile = new File(outFolder, version + ".pak");

        // check if existing pak directory is actually directory, and pak file with matching version
        // for this build is there, if not then delete all files residing in such
        // directory (old pak files) preparing for updating new one
        if (outFolder.isDirectory() && !outFile.exists()) // if local folder true and file does not match version empty folder
        {
          toast = "Updating please wait!";
          String[] children = outFolder.list();
          if (children != null) {
            for (String child : children) {
              new File(outFolder, child).delete();
            }
          }
        }
        else
        {
          toast = "First time setup, please wait...";
        }

        if (!outFile.exists())
        {
          Toast.makeText(appCtx, toast, Toast.LENGTH_LONG).show();
          outFolder.mkdirs();

		  //custom pak should be saved in "app\src\main\assets\bor.pak"
		  InputStream in = ctx.getAssets().open("bor.pak");
          FileOutputStream out = new FileOutputStream(outFile);

          copyFile(in, out);
          in.close();
          out.flush();
          out.close();
        }
      }
    } catch (Exception e) {
      // not handled
    }
  }

  private void copyFile(InputStream in, OutputStream out) throws IOException {
    byte[] buffer = new byte[1024];
    int read;
    while ((read = in.read(buffer)) != -1)
    {
      out.write(buffer, 0, read);
    }
  }

  @Override
  public void onLowMemory() {
    super.onLowMemory();
    Log.v("OpenBOR", "onLowMemory");

    //CRxTRDude - Release wakelock first before destroying.
    if (GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.release();
  }

  @Override
  protected void onPause() {
    super.onPause();
    Log.v("OpenBOR", "onPause");

    //White Dragon: wakelock release!
    if (GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.release();
  }

  @Override
  protected void onResume() {
    super.onResume();
    Log.v("OpenBOR", "onResume");

    //White Dragon: wakelock acquire!
    if (!GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.acquire(10*60*1000L /*10 minutes*/);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    Log.v("OpenBOR", "onDestroy");

    //CRxTRDude - Release wakelock first before destroying.
    if (GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.release();
  }
}
