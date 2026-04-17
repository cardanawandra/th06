package com.th06.game;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class JoystickView extends View {

    private float centerX, centerY;
    private float knobX, knobY;
    private float radius;

    private Paint basePaint = new Paint();
    private Paint knobPaint = new Paint();

    public interface JoystickListener {
        void onMove(Direction direction);
    }

    private JoystickListener listener;

    public void setListener(JoystickListener listener) {
        this.listener = listener;
    }

    public enum Direction {
        UP, DOWN, LEFT, RIGHT,
        UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT,
        CENTER
    }

    public JoystickView(Context context, AttributeSet attrs) {
        super(context, attrs);

        basePaint.setColor(Color.GRAY);
        basePaint.setAlpha(120);

        knobPaint.setColor(Color.BLUE);
        knobPaint.setAlpha(200);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        centerX = w / 2f;
        centerY = h / 2f;
        knobX = centerX;
        knobY = centerY;
        radius = Math.min(w, h) / 2f;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        // Base circle
        canvas.drawCircle(centerX, centerY, radius, basePaint);

        // Knob
        canvas.drawCircle(knobX, knobY, radius / 2, knobPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {

        float dx = event.getX() - centerX;
        float dy = event.getY() - centerY;

        double distance = Math.sqrt(dx * dx + dy * dy);

        double deadZone = radius * 0.2; // <-- sensitivity control

        // Clamp knob inside circle
        if (distance < radius) {
            knobX = event.getX();
            knobY = event.getY();
        } else {
            knobX = (float) (centerX + dx / distance * radius);
            knobY = (float) (centerY + dy / distance * radius);
        }

        if (event.getAction() == MotionEvent.ACTION_UP) {
            knobX = centerX;
            knobY = centerY;
            if (listener != null) listener.onMove(Direction.CENTER);
        } else {

            if (distance < deadZone) {
                if (listener != null) listener.onMove(Direction.CENTER);
                invalidate();
                return true;
            }

            double angle = Math.toDegrees(Math.atan2(dy, dx));
            if (angle < 0) angle += 360;

            Direction direction;

            if (angle >= 337.5 || angle < 22.5) direction = Direction.RIGHT;
            else if (angle < 67.5) direction = Direction.DOWN_RIGHT;
            else if (angle < 112.5) direction = Direction.DOWN;
            else if (angle < 157.5) direction = Direction.DOWN_LEFT;
            else if (angle < 202.5) direction = Direction.LEFT;
            else if (angle < 247.5) direction = Direction.UP_LEFT;
            else if (angle < 292.5) direction = Direction.UP;
            else direction = Direction.UP_RIGHT;

            if (listener != null) listener.onMove(direction);
        }

        invalidate();
        return true;
    }
}