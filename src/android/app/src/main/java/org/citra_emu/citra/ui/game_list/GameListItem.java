package org.citra_emu.citra.ui.game_list;

import android.content.Context;
import android.support.v7.widget.CardView;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.TextView;

import org.citra_emu.citra.R;

public final class GameListItem extends CardView {
    public void setTitle(String text) {
        title.setText(text);
    }

    private TextView title;
    private ImageView icon;

    public GameListItem(Context context) {
        super(context);
    }

    public GameListItem(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public GameListItem(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        title = findViewById(R.id.game_title);
        icon = findViewById(R.id.game_icon);
    }
}
