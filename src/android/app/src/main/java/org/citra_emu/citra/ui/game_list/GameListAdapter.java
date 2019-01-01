package org.citra_emu.citra.ui.game_list;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.citra_emu.citra.R;

public class GameListAdapter extends RecyclerView.Adapter<GameListAdapter.GameListViewHolder> {
    private GameListData[] games;

    public static class GameListViewHolder extends RecyclerView.ViewHolder {
        public GameListItem gameListItem;
        public GameListViewHolder(GameListItem v) {
            super(v);
            gameListItem = v;
        }
    }

    public GameListAdapter(GameListData[] g) {
        games = g;
    }

    @Override
    public GameListAdapter.GameListViewHolder onCreateViewHolder(ViewGroup parent,
                                                                 int viewType) {
        GameListItem v = (GameListItem) LayoutInflater.from(parent.getContext())
                .inflate(R.layout.game_list_item, parent, false);

        return new GameListViewHolder(v);
    }

    @Override
    public void onBindViewHolder(GameListViewHolder holder, int position) {
        GameListData game = games[position];
        holder.gameListItem.setTitle(game.title);

    }

    @Override
    public int getItemCount() {
        return games.length;
    }
}
