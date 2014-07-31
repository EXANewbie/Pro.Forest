#ifndef CONSTANT_H
#define CONSTANT_H

const int Port = 78911;
const int NOT_JOINED = -13;
const int UNDEFINED = -71;

const int HEADER_SIZE = 2 * sizeof(int);

const int BLOCK_COUNT = 100000;
const int BLOCK_SIZE = (1 << 14);

const int HANDLER_SIZE = 100000;

const int WIDTH = 500, HEIGHT = 500;

const int MINIMUM_SET_USER_BLOCK_SIZE = 12;
const int SET_USER_MAXIMUM = (2 * BLOCK_SIZE) / (3 * MINIMUM_SET_USER_BLOCK_SIZE);

const int MINIMUM_ERASE_USER_BLOCK_SIZE = 4;
const int ERASE_USER_MAXIMUM = BLOCK_SIZE / (2 * MINIMUM_ERASE_USER_BLOCK_SIZE);

const int MINIMUM_MOVE_USER_BLOCK_SIZE = 12;
const int MOVE_USER_MAXIMUM = (2 * BLOCK_SIZE) / (3 * MINIMUM_MOVE_USER_BLOCK_SIZE);

#endif