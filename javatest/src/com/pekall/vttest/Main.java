package com.pekall.vttest;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.util.Log;
import android.content.Intent;
import java.lang.CharSequence;


public class Main extends Activity {
    private static final String TAG = "VtTest";

    public static final int MENU_ID_PREVIEW = 1;
    public static final String MENU_NAME_PREVIEW = "Camera Preview";

	/**
	 * @see android.app.Activity#onCreate(Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
	}

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // create camera preview menu
        menu.add(0, MENU_ID_PREVIEW, 0, MENU_NAME_PREVIEW);
        
        return true;
    } 

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
        case MENU_ID_PREVIEW:
            Log.v(TAG, "enter camera preview menu");
            //Intent preview = new Intent(this, CameraPreview.class);
            Intent preview = new Intent(this, VideoTest.class);
            startActivity(preview);
            return true;
        default:
            Log.v(TAG, "ERROR, no such menu");
            break;
        }
        return false;
    }
}
