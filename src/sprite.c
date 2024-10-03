#include "sprite.h"

struct Sprite getSprite(enum SpriteID id)
{
	struct Sprite sprite = { 0 };
	switch (id) {
		case PLAYER_IDLE:
			sprite.width = 120;
			sprite.height = 80;
			sprite.num_frames = 10;
			sprite.x = 1;
			sprite.y = 739;
			break;
		case PLAYER_RUN:
			sprite.width = 120;
			sprite.height = 80;
			sprite.num_frames = 10;
			sprite.x = 1;
			sprite.y = 903;
			break;
		case PLAYER_CROUCH:
			sprite.width = 120;
			sprite.height = 80;
			sprite.num_frames = 1;
			sprite.x = 1143;
			sprite.y = 83;
			break;
		case PLAYER_CROUCH_WALK:
			sprite.width = 120;
			sprite.height = 80;
			sprite.num_frames = 8;
			sprite.x = 363;
			sprite.y = 411;
			break;
		case PLAYER_JUMP:
			sprite.width = 120;
			sprite.height = 80;
			sprite.num_frames = 3;
			sprite.x = 605;
			sprite.y = 493;
			break;
		case TILE_ROCK:
			sprite.width = 16;
			sprite.height = 16;
			sprite.num_frames = 1;
			sprite.x = 17;
			sprite.y = 1;
			break;
	}
	return sprite;
}