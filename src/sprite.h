#ifndef SPRITE_H
#define SPRITE_H

struct Sprite {
    int x, y; // spritesheet pos (top-left is origin)
    int width, height;
    int num_frames; // num of animated frames
};

enum SpriteID {
    PLAYER_IDLE,
    PLAYER_RUN,
    PLAYER_CROUCH,
    PLAYER_CROUCH_WALK,
    PLAYER_JUMP,
    TILE_ROCK
};

struct Sprite getSprite(enum SpriteID id);


#endif
