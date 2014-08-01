#include "monster.h"
#include "DMap.h"

bool Boundary_Check(const int, const int, const int, int, int);
void make_vector_id_in_room(E_List *, vector<int>&);

void Knight::getNextOffset(int bef_x_off, int bef_y_off, int *nxt_x_off, int *nxt_y_off)
{
	int Points[][2] = { { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 } };
	int size = 4;
//	for (int i = 0; i < 4;i++) 
	*nxt_x_off = bef_x_off;
	*nxt_y_off = bef_y_off;

	if (Boundary_Check(ID, x, y, *nxt_x_off, *nxt_y_off) == true)
	{
		return; // 
	}

	int *cur[] = { nxt_x_off, nxt_y_off };
	int nowPos = 0;
	for (int i = 0; i < size; i++)
	{
		i = nowPos;
		if (*cur[0] == Points[i][0] && *cur[1] == Points[i][1])
			break;
	}

	for (int i = 0; i < size; i++)
	{
		nowPos = (nowPos + 1) % size;
		*cur[0] = Points[nowPos][0];
		*cur[1] = Points[nowPos][1];
		if (Boundary_Check(ID, x, y, *nxt_x_off, *nxt_y_off) == true)
		{
			return;
		}
	}
}
