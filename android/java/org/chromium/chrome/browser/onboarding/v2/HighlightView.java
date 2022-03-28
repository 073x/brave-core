/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.graphics.BlurMaskFilter;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.FrameLayout;

import androidx.annotation.Keep;
import androidx.core.content.ContextCompat;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.ui.base.DeviceFormFactor;

public class HighlightView extends FrameLayout {

    private static final int ALPHA_60_PERCENT = 153;
    private static final int DEFAULT_ANIMATION_DURATION = 1000;

    private Paint eraserPaint;
    private Paint basicPaint;

    private HighlightItem item;
    private int itemWidth;
    private int itemHeight;

    private boolean shouldShowHighlight;
    private boolean isAnimating;
    private AnimatorSet animatorSet;

    private Context context;

    private float mInnerRadius;
    private float mOuterRadius;
    private int mColor = -1;

    public HighlightView(Context context, AttributeSet attrs) {
        super(context, attrs);

        this.context = context;
        PorterDuffXfermode xfermode = new PorterDuffXfermode(PorterDuff.Mode.MULTIPLY);
        basicPaint = new Paint();
        eraserPaint = new Paint();
        if (mColor == -1) {
            mColor = ContextCompat.getColor(context, android.R.color.white);
        }
        eraserPaint.setColor(mColor); //(0xFFFFFF);
        eraserPaint.setAlpha(0);
        eraserPaint.setXfermode(xfermode);
        eraserPaint.setAntiAlias(true);
    }

    public void setColor(int color) {
        mColor = color;
        invalidate();
    }

    public void setHighlightItem(HighlightItem item) {
        this.item = item;
        itemWidth = item.getScreenRight() - item.getScreenLeft();
        itemHeight = item.getScreenBottom() - item.getScreenTop();

        float radius = itemWidth > itemHeight ? ((float) itemWidth / 2) : ((float) itemHeight / 2);
        setInnerRadius(radius);
        setOuterRadius(radius);

        invalidate();
    }

    public void setShouldShowHighlight(boolean shouldShowHighlight) {
        this.shouldShowHighlight = shouldShowHighlight;
    }

    public void initializeAnimators() {
        animatorSet = new AnimatorSet();

        ObjectAnimator scaleXAnimator = ObjectAnimator.ofFloat(
                this, "innerRadius", mInnerRadius * 0.7f, mInnerRadius * 1.1f);
        scaleXAnimator.setRepeatCount(ValueAnimator.INFINITE);

        ObjectAnimator scaleBigAnimator = ObjectAnimator.ofFloat(
                this, "outerRadius", mOuterRadius * 0.9f, mOuterRadius * 1.2f);//1.2f, mOuterRadius * 1.4f);
        scaleBigAnimator.setRepeatCount(ValueAnimator.INFINITE);

        animatorSet.setDuration(DEFAULT_ANIMATION_DURATION);
        animatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        animatorSet.playTogether(scaleXAnimator, scaleBigAnimator);
    }

    public void startAnimation() {
        if (isAnimating) {
            // already animating
            return;
        }
        animatorSet.start();
        isAnimating = true;
    }

    public void stopAnimation() {
        if (!isAnimating) {
            // already not animating
            return;
        }
        animatorSet.end();
        isAnimating = false;
    }

    @Keep
    public void setInnerRadius(float radius) {
        mInnerRadius = radius;
        invalidate();
    }

    @Keep
    public void setOuterRadius(float radius) {
        mOuterRadius = radius;
        invalidate();
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        int[] location = new int[2];
        getLocationOnScreen(location);
        Bitmap overlay = Bitmap.createBitmap(getMeasuredWidth(), getMeasuredHeight(),
                Bitmap.Config.ARGB_8888);
        Canvas overlayCanvas = new Canvas(overlay);
        overlayCanvas.drawColor(0xcc1E2029);

        if (item == null) {
            return;
        }

        Paint innerBorderPaint = new Paint();
        innerBorderPaint.setStyle(Paint.Style.STROKE);
        innerBorderPaint.setStrokeWidth(2); // set stroke width
        innerBorderPaint.setColor(mColor); // Color.parseColor("#FFFFFF")); // set stroke color
        innerBorderPaint.setAntiAlias(true);

        Paint outterBorderPaint = new Paint();
        outterBorderPaint.setStyle(Paint.Style.STROKE);
        outterBorderPaint.setStrokeWidth(3); // set stroke width
        outterBorderPaint.setColor(mColor); // Color.parseColor("#FFFFFF")); // set stroke color
        outterBorderPaint.setAntiAlias(true);

        if (shouldShowHighlight) {
            int cx = item.getScreenLeft() + itemWidth / 2 - location[0];
            int cy = item.getScreenTop() + itemHeight / 2 - location[1];

            eraserPaint.setAlpha(0);
            float innerRadiusScaleMultiplier = 0.7f;
            overlayCanvas.drawCircle(
                    cx, cy, mInnerRadius * innerRadiusScaleMultiplier, eraserPaint);
            overlayCanvas.drawCircle(
                    cx, cy, mInnerRadius * innerRadiusScaleMultiplier, innerBorderPaint);

            eraserPaint.setAlpha(255);//ALPHA_60_PERCENT);
            float outerRadiusScaleMultiplier = 1.2f;
            overlayCanvas.drawCircle(
                    cx, cy, mOuterRadius * outerRadiusScaleMultiplier, eraserPaint);
            overlayCanvas.drawCircle(
                    cx, cy, mOuterRadius * outerRadiusScaleMultiplier, outterBorderPaint);

        } else {
            boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);
            int verticalOffset = isTablet ? 35 : 80;
            eraserPaint.setAlpha(0);
            outterBorderPaint.setStrokeWidth(6);
            RectF innerRect = new RectF(item.getScreenLeft() + 10,
                    (item.getScreenTop() - verticalOffset) + 10, item.getScreenRight() - 10,
                    item.getScreenBottom() - (isTablet ? 45 : 10) - verticalOffset);
            overlayCanvas.drawRoundRect(innerRect, 12, 12, eraserPaint);
            // overlayCanvas.drawRoundRect(innerRect, 12, 12, innerBorderPaint);

            //eraserPaint.setAlpha(ALPHA_60_PERCENT);

            Paint shadowPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            shadowPaint.setStyle(Paint.Style.FILL);
            shadowPaint.setColor(0x982D98); // Color.parseColor("#FFFFFF")); // set stroke color
            //shadowPaint.setAntiAlias(true);

            shadowPaint.setMaskFilter(new BlurMaskFilter(
            22 /* shadowRadius */,
            BlurMaskFilter.Blur.NORMAL));

            RectF outerRect = new RectF((float) item.getScreenLeft(), (float) (item.getScreenTop() - verticalOffset),
                    (float)item.getScreenRight(),
                    (float)(item.getScreenBottom() - (isTablet ? 35 : 0) - verticalOffset));

            // Draw shadow before drawing object
            overlayCanvas.drawRoundRect(outerRect, 22, 22, shadowPaint);
            overlayCanvas.drawRoundRect(outerRect, 22, 22, eraserPaint);
            overlayCanvas.drawRoundRect(outerRect, 22, 22, outterBorderPaint);
            shadowPaint.setMaskFilter(null);
            
            //overlayCanvas.drawRoundRect(outerRect, 22, 22, outterBorderPaint);
        }
        // mShadow.setShadowLayer(10.0f, 0.0f, 2.0f, 0xFF000000);
        canvas.drawBitmap(overlay, 0, 0, basicPaint);
        super.dispatchDraw(canvas);
    }
}
