/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ImageView;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.ScaleAnimation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;

public class OnboardingActivity2 extends AppCompatActivity {

    private ImageView mIvLeafTop;
    private ImageView mIvLeafBottom;
    private View mVLeafAlignTop;
    private View mVLeafAlignBottom;
    private ImageView mIvBraveTop;
    private ImageView mIvBraveBottom;
    private ImageView mIvArrowUp;
    private ImageView mIvArrowDown;
    private TextView mTvWelcomeTitle;
    private TextView mTvTitle;
    private TextView mTvDesc;
    private Button mBtnPositive;
    private Button mBtnNegative;
    private Button mBtnSkip;

    private int mCurrentStep = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_onboarding2);

        initViews();
        onClickViews();
        startTimer();
    }

    private void initViews() {

        mIvLeafTop = findViewById(R.id.iv_leaf_top);
        mIvLeafBottom = findViewById(R.id.iv_leaf_bottom);
        mVLeafAlignTop = findViewById(R.id.view_leaf_top_align);
        mVLeafAlignBottom = findViewById(R.id.view_leaf_bottom_align);
        mIvBraveTop = findViewById(R.id.iv_brave_top);
        mIvBraveBottom = findViewById(R.id.iv_brave_bottom);
        mIvArrowUp = findViewById(R.id.iv_arrow_up);
        mIvArrowDown = findViewById(R.id.iv_arrow_down);
        mTvWelcomeTitle = findViewById(R.id.tv_welcome_title);
        mTvTitle = findViewById(R.id.tv_title);
        mTvDesc = findViewById(R.id.tv_desc);
        mBtnPositive = findViewById(R.id.btn_positive);
        mBtnNegative = findViewById(R.id.btn_negative);
        mBtnSkip = findViewById(R.id.btn_skip);
    }

    private void onClickViews() {

        mBtnPositive.setOnClickListener(view -> {

            if(mCurrentStep == 2) {
                BraveSetDefaultBrowserUtils.showBraveSetDefaultBrowserDialog(this, false);
            }
            displayNext();
        });

        mBtnNegative.setOnClickListener(view -> {

            displayNext();
        });

        mBtnSkip.setOnClickListener(view -> {

            if(BraveActivity.getBraveActivity() != null) {
                Log.e("tapan","brave is not null");
            } else {
                Log.e("tapan","brave is null");
            }
        });
    }

    private void startTimer() {

        Handler handler = new Handler();
        handler.postDelayed(this::displayNext, 3000);
    }

    private void displayNext() {

        mCurrentStep++;

        if(mCurrentStep == 1) {
            
            mBtnSkip.setVisibility(View.VISIBLE);
            mTvWelcomeTitle.setVisibility(View.GONE);
            mTvTitle.setVisibility(View.VISIBLE);
            mTvDesc.setVisibility(View.VISIBLE);
            mBtnPositive.setVisibility(View.VISIBLE);

            mTvTitle.setText(getResources().getString(R.string.privacy_onboarding));
            mTvDesc.setText(getResources().getString(R.string.privacy_desc_onboarding));
            mBtnPositive.setText(getResources().getString(R.string.lets_go));

            setLeafView(1, 1.3f, 40);
            
        } else if(mCurrentStep == 2) {

            mBtnNegative.setVisibility(View.VISIBLE);
            
            mIvBraveBottom.setImageResource(R.drawable.ic_phone_onboarding);
            mTvTitle.setText(getResources().getString(R.string.set_default_browser_title));
            mTvDesc.setText(getResources().getString(R.string.set_default_browser_text));
            mBtnPositive.setText(getResources().getString(R.string.set_brave_default_browser));
            mBtnNegative.setText(getResources().getString(R.string.not_now));

            setLeafView(1.3f, 1.6f, 80);

        } else if(mCurrentStep == 3) {

            mIvBraveBottom.setVisibility(View.GONE);
            mIvArrowDown.setVisibility(View.GONE);
            mIvBraveTop.setVisibility(View.VISIBLE);
            mIvArrowUp.setVisibility(View.VISIBLE);
            mBtnPositive.setVisibility(View.GONE);
            mBtnNegative.setVisibility(View.GONE);

            mTvTitle.setText(getResources().getString(R.string.ready_browse));
            mTvDesc.setText(getResources().getString(R.string.select_popular_site));

            setLeafView(1.6f, 2f, 120);

            setSitesRecyclerView();
        }
    }

    private void setSitesRecyclerView() {

        RecyclerView rvSites = findViewById(R.id.recyclerview_sites);
        rvSites.setVisibility(View.VISIBLE);
        rvSites.setLayoutManager(new LinearLayoutManager(this));
        OnboardingSitesAdapter onboardingSitesAdapter = new OnboardingSitesAdapter();
        rvSites.setAdapter(onboardingSitesAdapter);
    }

    private void setLeafView(float lastScale, float scale, int leafMargin) {

        //mIvLeafTop.setScaleX(scale);
        //mIvLeafTop.setScaleY(scale);
        //Log.e("tapan","scale:"+mIvLeafTop.getScaleX()+","+mIvLeafTop.getScaleY());
        Animation anim = new ScaleAnimation(
            lastScale, scale, // Start and end values for the X axis scaling
            lastScale, scale, // Start and end values for the Y axis scaling
            Animation.RELATIVE_TO_SELF, 0.5f, // Pivot point of X scaling
            Animation.RELATIVE_TO_SELF, 0.5f); // Pivot point of Y scaling
        anim.setFillAfter(true); // Needed to keep the result of the animation
        anim.setDuration(50);
        mIvLeafTop.startAnimation(anim);

        //mIvLeafBottom.setScaleX(scale);
        //mIvLeafBottom.setScaleY(scale);

        ViewGroup.MarginLayoutParams topLeafLayoutParams = (ViewGroup.MarginLayoutParams) mVLeafAlignTop.getLayoutParams();
        topLeafLayoutParams.bottomMargin = dpToPx(this, leafMargin);
        mVLeafAlignTop.setLayoutParams(topLeafLayoutParams);

        /*ViewGroup.MarginLayoutParams bottomLeafLayoutParams = (ViewGroup.MarginLayoutParams) mVLeafAlignBottom.getLayoutParams();
        bottomLeafLayoutParams.topMargin = dpToPx(this, leafMargin);
        mVLeafAlignBottom.setLayoutParams(bottomLeafLayoutParams); */   
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK
                && requestCode == BraveSetDefaultBrowserUtils.DEFAULT_BROWSER_ROLE_REQUEST_CODE) {
            BraveSetDefaultBrowserUtils.setBraveDefaultSuccess();
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onBackPressed() {}


}
