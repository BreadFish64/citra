package org.citra_emu.citra.ui.game_list;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;

public class GameList extends RecyclerView {
    public GameList(Context context) {
        super(context);
    }

    public GameList(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public GameList(Context context, @Nullable AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        setLayoutManager(new LinearLayoutManager(getContext()));

        addItemDecoration(new SpaceItemDecoration(32, 32));

        GameListData[] meep = {
                new GameListData("Old"), new GameListData("MacDonald"), new GameListData("Had"), new GameListData("A"), new GameListData("Farm"),
                new GameListData("e"), new GameListData("i"), new GameListData("e"), new GameListData("i"), new GameListData("o")
        };

        setAdapter(new GameListAdapter(meep));
    }
}
