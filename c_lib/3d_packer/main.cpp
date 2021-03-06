#define _CRT_SECURE_NO_WARNINGS

extern "C"
{

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#define TRUE 1
#define FALSE 0

#define SKU_size 8



void __cdecl initialize(void);
void __cdecl read_input_file(void);
void __cdecl execute_iterations(void);
void __cdecl list_candidate_layers(void);
int __cdecl compute_layer_list(const void* i, const void* j);
int __cdecl pack_layer(void);
int __cdecl find_layer(short int thickness);
void __cdecl find_box(short int hmx, short int hy, short int hmy, short int hz, short int hmz);
void __cdecl analyze_box(short int hmx, short int hy, short int hmy, short int hz, short int hmz, short int dim1, short int dim2, short int dim3);
void __cdecl find_smallest_z(void);
void __cdecl check_if_found(void);
void __cdecl volume_check(void);
void __cdecl write_visualization_data_file(void);
void __cdecl write_boxlist_file(void);
void __cdecl report_results(void);



char str_p_x[5], str_p_y[5], str_p_z[5];
char str_co_x[5], str_co_y[5], str_co_z[5];
char str_SKU[SKU_size];
char str_weight[5];
char str_pack_x[5], str_pack_y[5], str_pack_z[5], str_pack_w[5];

char *filename = NULL;
char packing;
char layer_done;
char evened;
char variant;
char best_variant;
char packing_best;
char hundred_percent;
char graphout[] = ".~3d";
char unpacked;
char *not_included_list[];

short int box_x, box_y, box_z, box_i;
short int b_box_x, b_box_y, b_box_z, b_box_i;
short int current_box_x, current_box_y, current_box_z, current_box_i;
short int bf_x, bf_y, bf_z;
short int b_bf_x, b_bf_y, b_bf_z;
short int xx, yy, zz;
short int pallet_x, pallet_y, pallet_z;

short int all_boxes;
short int x;
short int n;
short int layer_listlen;
short int layer_in_layer;
short int prelayer;
short int lilz;
short int number_of_iterations;
short int hour; //  |
short int min;  //  | Time
short int sec;  //  |
short int layers_index;
short int remain_p_x, remain_p_y, remain_p_z;
short int packed_y;
short int prepacked_y;
short int layer_thickness;
short int itelayer;
short int preremain_p_y;
short int best_iteration;
short int packed_numbox;
short int number_packed_boxes;

double packed_volume;
double best_solution_volume;
double total_pallet_volume;
double total_box_volume;
double vol_coefficient;
double pallet_volume_used_percentage;
double packed_box_percentage;
double calc_time;


struct boxinfo {
	char is_packed;
	short int dim1, dim2, dim3, n, co_x, co_y, co_z, pack_x, pack_y, pack_z;
	char SKU[SKU_size] = { 0 }; // Yes, not cleanin' all char array, but nevermind
	int weight;
	long int volume;
} boxlist[5000]; // Limit 5k

struct layerlist {
	long int layer_eval;
	short int layer_dim;
} layers[1000]; // Limit 1k

struct scrappad {
	struct scrappad* prev, * next;
	short int cum_x, cum_z;
};

struct scrappad *scrap_first, *scrap_member, *smallest_z, *trash;

time_t start, finish; // Timer

FILE *boxlist_input_file, *report_output_file, *visualizer_file; // File descriptors, but in future need to del

char version[] = "0.03 dynamic lib version"; // With dopil:)


// Lauch func
int run(char* param_filename)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)

	// TODO: Add russian lang support, but not now

	//Parse Command line options
	memcpy ( &filename, &param_filename, sizeof(param_filename) );


	initialize();
	time(&start);
	execute_iterations();
	time(&finish);
	report_results();
	return(0);
}


// Init
void initialize(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	read_input_file();
	vol_coefficient = 1.0;
	total_pallet_volume = vol_coefficient * xx * yy * zz;
	total_box_volume = 0.0;
	for (x = 1; x <= all_boxes; x++) {
		total_box_volume = total_box_volume + boxlist[x].volume;
	}

	scrap_first = (scrappad*)malloc(sizeof(struct scrappad));
	if (scrap_first == NULL)
	{
		printf("Insufficient memory available\n");
		exit(1);
	}

	scrap_first->prev = NULL;
	scrap_first->next = NULL;
	best_solution_volume = 0.0;
	packing_best = 0;
	hundred_percent = 0;
	number_of_iterations = 0;
}


// Read input file (now: copy of box.txt)
void read_input_file(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	short int n;
	//TODO: Robustify this so the box label can be larger and have spaces
	char lbl[SKU_size], dim1[5], dim2[5], dim3[5], str_w[5], box_n[5], str_xx[5], str_yy[5], str_zz[5];

	if ((boxlist_input_file = fopen(filename, "r")) == NULL)
	{
		printf("Cannot open file %s\n", filename);
		exit(1);
	}
	all_boxes = 1;

	if (fscanf(boxlist_input_file, "%s %s %s", str_xx, str_yy, str_zz) == EOF)
	{
		exit(1);
	}

	xx = atoi(str_xx);
	yy = atoi(str_yy);
	zz = atoi(str_zz);

	while (fscanf(boxlist_input_file, "%s %s %s %s %s %s", lbl, dim1, dim2, dim3, str_w, box_n) != EOF)
	{
		boxlist[all_boxes].dim1 = atoi(dim1);
		boxlist[all_boxes].dim2 = atoi(dim2);
		boxlist[all_boxes].dim3 = atoi(dim3);
		boxlist[all_boxes].weight = atoi(str_w); // Add Weight

		// Add SKU label
		strncpy(boxlist[all_boxes].SKU, lbl, SKU_size);

		boxlist[all_boxes].volume = boxlist[all_boxes].dim1 * boxlist[all_boxes].dim2 * boxlist[all_boxes].dim3;
		n = atoi(box_n);
		boxlist[all_boxes].n = n;

		while (--n)
		{
			boxlist[all_boxes + n] = boxlist[all_boxes];
		}
		all_boxes = all_boxes + atoi(box_n);
	}
	--all_boxes;
	fclose(boxlist_input_file);
	return;
}


// If iterations are done and found best solution
void execute_iterations(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	short int for_counter = 2; // May be 6 if all variants is relevant
	for (variant = 1; variant <= for_counter; variant++)
	{

		// sick!
		switch (variant)
		{
		case 1: // Default
			pallet_x = xx; pallet_y = yy; pallet_z = zz;
			break;
		case 2: // Rotate 90
			pallet_x = zz; pallet_y = yy; pallet_z = xx;
			break;
		}

		

		list_candidate_layers();
		layers[0].layer_eval = -1;
		qsort(layers, layer_listlen + 1, sizeof(struct layerlist), compute_layer_list);

		for (layers_index = 1; layers_index <= layer_listlen; layers_index++)
		{
			++number_of_iterations;
			time(&finish);
			calc_time = difftime(finish, start);

			// sick!
			// printf("VARIANT: %5d; ITERATION (TOTAL): %5d; BEST SO FAR: %.3f %%; TIME: %.0f", variant, number_of_iterations, pallet_volume_used_percentage, calc_time);
			packed_volume = 0.0;
			packed_y = 0;
			packing = 1;
			layer_thickness = layers[layers_index].layer_dim;
			itelayer = layers_index;
			remain_p_y = pallet_y;
			remain_p_z = pallet_z;
			packed_numbox = 0;
			for (x = 1; x <= all_boxes; x++)
			{
				boxlist[x].is_packed = FALSE;
			}

			//BEGIN DO-WHILE
			do
			{
				layer_in_layer = 0;
				layer_done = 0;
				if (pack_layer())
				{
					exit(1);
				}
				packed_y = packed_y + layer_thickness;
				remain_p_y = pallet_y - packed_y;
				if (layer_in_layer)
				{
					prepacked_y = packed_y;
					preremain_p_y = remain_p_y;
					remain_p_y = layer_thickness - prelayer;
					packed_y = packed_y - layer_thickness + prelayer;
					remain_p_z = lilz;
					layer_thickness = layer_in_layer;
					layer_done = 0;
					if (pack_layer())
					{
						exit(1);
					}
					packed_y = prepacked_y;
					remain_p_y = preremain_p_y;
					remain_p_z = pallet_z;
				}
				find_layer(remain_p_y);
			} while (packing);
			// END DO-WHILE

			if (packed_volume > best_solution_volume)
			{
				best_solution_volume = packed_volume;
				best_variant = variant;
				best_iteration = itelayer;
				number_packed_boxes = packed_numbox;
			}

			if (hundred_percent) break;
			pallet_volume_used_percentage = best_solution_volume * 100 / total_pallet_volume;
			// Some nondestructive backspace
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		}
		if (hundred_percent) break;
		if ((xx == yy) && (yy == zz)) variant = 6;
	}
}


// List of all possible layer heights by giving a weight to each of them
void list_candidate_layers(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	char same;
	short int ex_dim, dim_dif, dimen_2, dimen_3, y, z, k;
	long int layereval;

	layer_listlen = 0;

	for (x = 1; x <= all_boxes; x++)
	{
		for (y = 1; y <= 3; y++)
		{
			switch (y)
			{
			case 1:
				ex_dim = boxlist[x].dim1;
				dimen_2 = boxlist[x].dim2;
				dimen_3 = boxlist[x].dim3;
				break;
			case 2:
				ex_dim = boxlist[x].dim2;
				dimen_2 = boxlist[x].dim1;
				dimen_3 = boxlist[x].dim3;
				break;
			case 3:
				ex_dim = boxlist[x].dim3;
				dimen_2 = boxlist[x].dim1;
				dimen_3 = boxlist[x].dim2;
				break;
			}
			if ((ex_dim > pallet_y) || (((dimen_2 > pallet_x) || (dimen_3 > pallet_z)) && ((dimen_3 > pallet_x) || (dimen_2 > pallet_z)))) continue;
			same = 0;

			for (k = 1; k <= layer_listlen; k++)
			{
				if (ex_dim == layers[k].layer_dim)
				{
					same = 1;
					continue;
				}
			}
			if (same) continue;
			layereval = 0;
			for (z = 1; z <= all_boxes; z++)
			{
				if (!(x == z))
				{
					dim_dif = abs(ex_dim - boxlist[z].dim1);
					if (abs(ex_dim - boxlist[z].dim2) < dim_dif)
					{
						dim_dif = abs(ex_dim - boxlist[z].dim2);
					}
					if (abs(ex_dim - boxlist[z].dim3) < dim_dif)
					{
						dim_dif = abs(ex_dim - boxlist[z].dim3);
					}
					layereval = layereval + dim_dif;
				}
			}
			layers[++layer_listlen].layer_eval = layereval;
			layers[layer_listlen].layer_dim = ex_dim;
		}
	}
	return;
}


// Simple wrap for quick sort
int compute_layer_list(const void* i, const void* j)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	return *(long int*)i - *(long int*)j;
}


// Pack all boxes and right record all records and vars
int pack_layer(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	short int len_x, len_z, l_p_z;

	if (!layer_thickness)
	{
		packing = 0;
		return 0;
	}

	scrap_first->cum_x = pallet_x;
	scrap_first->cum_z = 0;

	while (1)
	{
		find_smallest_z();

		if (!smallest_z->prev && !smallest_z->next)
		{
			//*** SITUATION-1: NO BOXES ON THE RIGHT AND LEFT SIDES ***

			len_x = smallest_z->cum_x;
			l_p_z = remain_p_z - smallest_z->cum_z;
			find_box(len_x, layer_thickness, remain_p_y, l_p_z, l_p_z);
			check_if_found();

			if (layer_done) break;
			if (evened) continue;

			boxlist[current_box_i].co_x = 0;
			boxlist[current_box_i].co_y = packed_y;
			boxlist[current_box_i].co_z = smallest_z->cum_z;
			if (current_box_x == smallest_z->cum_x)
			{
				smallest_z->cum_z = smallest_z->cum_z + current_box_z;
			}
			else
			{
				smallest_z->next = (scrappad*)malloc(sizeof(struct scrappad));
				if (smallest_z->next == NULL)
				{
					printf("Insufficient memory available\n");
					return 1;
				}
				smallest_z->next->next = NULL;
				smallest_z->next->prev = smallest_z;
				smallest_z->next->cum_x = smallest_z->cum_x;
				smallest_z->next->cum_z = smallest_z->cum_z;
				smallest_z->cum_x = current_box_x;
				smallest_z->cum_z = smallest_z->cum_z + current_box_z;
			}
			volume_check();
		}
		else if (!smallest_z->prev)
		{
			//*** SITUATION-2: NO BOXES ON THE LEFT SIDE ***

			len_x = smallest_z->cum_x;
			len_z = smallest_z->next->cum_z - smallest_z->cum_z;
			l_p_z = remain_p_z - smallest_z->cum_z;
			find_box(len_x, layer_thickness, remain_p_y, len_z, l_p_z);
			check_if_found();

			if (layer_done) break;
			if (evened) continue;

			boxlist[current_box_i].co_y = packed_y;
			boxlist[current_box_i].co_z = smallest_z->cum_z;
			if (current_box_x == smallest_z->cum_x)
			{
				boxlist[current_box_i].co_x = 0;
				if (smallest_z->cum_z + current_box_z == smallest_z->next->cum_z)
				{
					smallest_z->cum_z = smallest_z->next->cum_z;
					smallest_z->cum_x = smallest_z->next->cum_x;
					trash = smallest_z->next;
					smallest_z->next = smallest_z->next->next;
					if (smallest_z->next)
					{
						smallest_z->next->prev = smallest_z;
					}
					free(trash);
				}
				else
				{
					smallest_z->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			else
			{
				boxlist[current_box_i].co_x = smallest_z->cum_x - current_box_x;
				if (smallest_z->cum_z + current_box_z == smallest_z->next->cum_z)
				{
					smallest_z->cum_x = smallest_z->cum_x - current_box_x;
				}
				else
				{
					smallest_z->next->prev = (scrappad*)malloc(sizeof(struct scrappad));
					if (smallest_z->next->prev == NULL)
					{
						printf("Insufficient memory available\n");
						return 1;
					}
					smallest_z->next->prev->next = smallest_z->next;
					smallest_z->next->prev->prev = smallest_z;
					smallest_z->next = smallest_z->next->prev;
					smallest_z->next->cum_x = smallest_z->cum_x;
					smallest_z->cum_x = smallest_z->cum_x - current_box_x;
					smallest_z->next->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			volume_check();
		}
		else if (!smallest_z->next)
		{
			//*** SITUATION-3: NO BOXES ON THE RIGHT SIDE ***

			len_x = smallest_z->cum_x - smallest_z->prev->cum_x;
			len_z = smallest_z->prev->cum_z - smallest_z->cum_z;
			l_p_z = remain_p_z - (*smallest_z).cum_z;
			find_box(len_x, layer_thickness, remain_p_y, len_z, l_p_z);
			check_if_found();

			if (layer_done) break;
			if (evened) continue;

			boxlist[current_box_i].co_y = packed_y;
			boxlist[current_box_i].co_z = smallest_z->cum_z;
			boxlist[current_box_i].co_x = smallest_z->prev->cum_x;

			if (current_box_x == smallest_z->cum_x - smallest_z->prev->cum_x)
			{
				if (smallest_z->cum_z + current_box_z == smallest_z->prev->cum_z)
				{
					smallest_z->prev->cum_x = smallest_z->cum_x;
					smallest_z->prev->next = NULL;
					free(smallest_z);
				}
				else
				{
					smallest_z->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			else
			{
				if (smallest_z->cum_z + current_box_z == smallest_z->prev->cum_z)
				{
					smallest_z->prev->cum_x = smallest_z->prev->cum_x + current_box_x;
				}
				else
				{
					smallest_z->prev->next = (scrappad*)malloc(sizeof(struct scrappad));
					if (smallest_z->prev->next == NULL)
					{
						printf("Insufficient memory available\n");
						return 1;
					}
					smallest_z->prev->next->prev = smallest_z->prev;
					smallest_z->prev->next->next = smallest_z;
					smallest_z->prev = smallest_z->prev->next;
					smallest_z->prev->cum_x = smallest_z->prev->prev->cum_x + current_box_x;
					smallest_z->prev->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			volume_check();
		}
		else if (smallest_z->prev->cum_z == smallest_z->next->cum_z)
		{
			//*** SITUATION-4: THERE ARE BOXES ON BOTH OF THE SIDES ***

			//*** SUBSITUATION-4A: SIDES ARE EQUAL TO EACH OTHER ***

			len_x = smallest_z->cum_x - smallest_z->prev->cum_x;
			len_z = smallest_z->prev->cum_z - smallest_z->cum_z;
			l_p_z = remain_p_z - smallest_z->cum_z;

			find_box(len_x, layer_thickness, remain_p_y, len_z, l_p_z);
			check_if_found();

			if (layer_done) break;
			if (evened) continue;

			boxlist[current_box_i].co_y = packed_y;
			boxlist[current_box_i].co_z = smallest_z->cum_z;
			if (current_box_x == smallest_z->cum_x - smallest_z->prev->cum_x)
			{
				boxlist[current_box_i].co_x = smallest_z->prev->cum_x;
				if (smallest_z->cum_z + current_box_z == smallest_z->next->cum_z)
				{
					smallest_z->prev->cum_x = smallest_z->next->cum_x;
					if (smallest_z->next->next)
					{
						smallest_z->prev->next = smallest_z->next->next;
						smallest_z->next->next->prev = smallest_z->prev;
						free(smallest_z);
					}
					else
					{
						smallest_z->prev->next = NULL;
						free(smallest_z);
					}
				}
				else
				{
					smallest_z->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			else if (smallest_z->prev->cum_x < pallet_x - smallest_z->cum_x)
			{
				if (smallest_z->cum_z + current_box_z == smallest_z->prev->cum_z)
				{
					smallest_z->cum_x = smallest_z->cum_x - current_box_x;
					boxlist[current_box_i].co_x = smallest_z->cum_x - current_box_x;
				}
				else
				{
					boxlist[current_box_i].co_x = smallest_z->prev->cum_x;
					smallest_z->prev->next = (scrappad*)malloc(sizeof(struct scrappad));
					if (smallest_z->prev->next == NULL)
					{
						printf("Insufficient memory available\n");
						return 1;
					}
					smallest_z->prev->next->prev = smallest_z->prev;
					smallest_z->prev->next->next = smallest_z;
					smallest_z->prev = smallest_z->prev->next;
					smallest_z->prev->cum_x = smallest_z->prev->prev->cum_x + current_box_x;
					smallest_z->prev->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			else
			{
				if (smallest_z->cum_z + current_box_z == smallest_z->prev->cum_z)
				{
					smallest_z->prev->cum_x = smallest_z->prev->cum_x + current_box_x;
					boxlist[current_box_i].co_x = smallest_z->prev->cum_x;
				}
				else
				{
					boxlist[current_box_i].co_x = smallest_z->cum_x - current_box_x;
					smallest_z->next->prev = (scrappad*)malloc(sizeof(struct scrappad));
					if (smallest_z->next->prev == NULL)
					{
						printf("Insufficient memory available\n");
						return 1;
					}
					smallest_z->next->prev->next = smallest_z->next;
					smallest_z->next->prev->prev = smallest_z;
					smallest_z->next = smallest_z->next->prev;
					smallest_z->next->cum_x = smallest_z->cum_x;
					smallest_z->next->cum_z = smallest_z->cum_z + current_box_z;
					smallest_z->cum_x = smallest_z->cum_x - current_box_x;
				}
			}
			volume_check();
		}
		else
		{
			//*** SUBSITUATION-4B: SIDES ARE NOT EQUAL TO EACH OTHER ***

			len_x = smallest_z->cum_x - smallest_z->prev->cum_x;
			len_z = smallest_z->prev->cum_z - smallest_z->cum_z;
			l_p_z = remain_p_z - smallest_z->cum_z;
			find_box(len_x, layer_thickness, remain_p_y, len_z, l_p_z);
			check_if_found();

			if (layer_done) break;
			if (evened) continue;

			boxlist[current_box_i].co_y = packed_y;
			boxlist[current_box_i].co_z = smallest_z->cum_z;
			boxlist[current_box_i].co_x = smallest_z->prev->cum_x;
			if (current_box_x == smallest_z->cum_x - smallest_z->prev->cum_x)
			{
				if (smallest_z->cum_z + current_box_z == smallest_z->prev->cum_z)
				{
					smallest_z->prev->cum_x = smallest_z->cum_x;
					smallest_z->prev->next = smallest_z->next;
					smallest_z->next->prev = smallest_z->prev;
					free(smallest_z);
				}
				else
				{
					smallest_z->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			else
			{
				if (smallest_z->cum_z + current_box_z == smallest_z->prev->cum_z)
				{
					smallest_z->prev->cum_x = smallest_z->prev->cum_x + current_box_x;
				}
				else if (smallest_z->cum_z + current_box_z == smallest_z->next->cum_z)
				{
					boxlist[current_box_i].co_x = smallest_z->cum_x - current_box_x;
					smallest_z->cum_x = smallest_z->cum_x - current_box_x;
				}
				else
				{
					smallest_z->prev->next = (scrappad*)malloc(sizeof(struct scrappad));
					if (smallest_z->prev->next == NULL)
					{
						printf("Insufficient memory available\n");
						return 1;
					}
					smallest_z->prev->next->prev = smallest_z->prev;
					smallest_z->prev->next->next = smallest_z;
					smallest_z->prev = smallest_z->prev->next;
					smallest_z->prev->cum_x = smallest_z->prev->prev->cum_x + current_box_x;
					smallest_z->prev->cum_z = smallest_z->cum_z + current_box_z;
				}
			}
			volume_check();
		}
	}
	return 0;
}


// Find the most suitable layer height by looking at unpacked boxrs and at remaining free space
int find_layer(short int thickness)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	short int exdim, dimdif, dimen2, dimen3, y, z;
	long int layereval, eval;
	layer_thickness = 0;
	eval = 1000000;
	for (x = 1; x <= all_boxes; x++)
	{
		if (boxlist[x].is_packed) continue;
		for (y = 1; y <= 3; y++)
		{
			switch (y)
			{
			case 1:
				exdim = boxlist[x].dim1;
				dimen2 = boxlist[x].dim2;
				dimen3 = boxlist[x].dim3;
				break;
			case 2:
				exdim = boxlist[x].dim2;
				dimen2 = boxlist[x].dim1;
				dimen3 = boxlist[x].dim3;
				break;
			case 3:
				exdim = boxlist[x].dim3;
				dimen2 = boxlist[x].dim1;
				dimen3 = boxlist[x].dim2;
				break;
			}
			layereval = 0;
			if ((exdim <= thickness) && (((dimen2 <= pallet_x) && (dimen3 <= pallet_z)) || ((dimen3 <= pallet_x) && (dimen2 <= pallet_z))))
			{
				for (z = 1; z <= all_boxes; z++)
				{
					if (!(x == z) && !(boxlist[z].is_packed))
					{
						dimdif = abs(exdim - boxlist[z].dim1);
						if (abs(exdim - boxlist[z].dim2) < dimdif)
						{
							dimdif = abs(exdim - boxlist[z].dim2);
						}
						if (abs(exdim - boxlist[z].dim3) < dimdif)
						{
							dimdif = abs(exdim - boxlist[z].dim3);
						}
						layereval = layereval + dimdif;
					}
				}
				if (layereval < eval)
				{
					eval = layereval;
					layer_thickness = exdim;
				}
			}
		}
	}
	if (layer_thickness == 0 || layer_thickness > remain_p_y) packing = 0;
	return 0;
}


// Find the most prober boxes by looking at possible orientations, empty space, adj. boxes and pallet limits
void find_box(short int hmx, short int hy, short int hmy, short int hz, short int hmz)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	short int y;
	bf_x = 32767; bf_y = 32767; bf_z = 32767;
	b_bf_x = 32767; b_bf_y = 32767; b_bf_z = 32767;
	box_i = 0; b_box_i = 0;
	for (y = 1; y <= all_boxes; y = y + boxlist[y].n)
	{
		for (x = y; x < x + boxlist[y].n - 1; x++)
		{
			if (!boxlist[x].is_packed) break;
		}
		if (boxlist[x].is_packed) continue;
		if (x > all_boxes) return;
		analyze_box(hmx, hy, hmy, hz, hmz, boxlist[x].dim1, boxlist[x].dim2, boxlist[x].dim3);
		if ((boxlist[x].dim1 == boxlist[x].dim3) && (boxlist[x].dim3 == boxlist[x].dim2)) continue;
		analyze_box(hmx, hy, hmy, hz, hmz, boxlist[x].dim3, boxlist[x].dim2, boxlist[x].dim1);
		// If need 6 variants of orientation, need to add at this place some rules
	}
}


// Analys each upacked box to find best place to the empty space
void analyze_box(short int hmx, short int hy, short int hmy, short int hz, short int hmz, short int dim1, short int dim2, short int dim3)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	if (dim1 <= hmx && dim2 <= hmy && dim3 <= hmz)
	{
		if (dim2 <= hy)
		{
			if (hy - dim2 < bf_y)
			{
				box_x = dim1;
				box_y = dim2;
				box_z = dim3;
				bf_x = hmx - dim1;
				bf_y = hy - dim2;
				bf_z = abs(hz - dim3);
				box_i = x;
			}
			else if (hy - dim2 == bf_y && hmx - dim1 < bf_x)
			{
				box_x = dim1;
				box_y = dim2;
				box_z = dim3;
				bf_x = hmx - dim1;
				bf_y = hy - dim2;
				bf_z = abs(hz - dim3);
				box_i = x;
			}
			else if (hy - dim2 == bf_y && hmx - dim1 == bf_x && abs(hz - dim3) < bf_z)
			{
				box_x = dim1;
				box_y = dim2;
				box_z = dim3;
				bf_x = hmx - dim1;
				bf_y = hy - dim2;
				bf_z = abs(hz - dim3);
				box_i = x;
			}
		}
		else
		{
			if (dim2 - hy < b_bf_y)
			{
				b_box_x = dim1;
				b_box_y = dim2;
				b_box_z = dim3;
				b_bf_x = hmx - dim1;
				b_bf_y = dim2 - hy;
				b_bf_z = abs(hz - dim3);
				b_box_i = x;
			}
			else if (dim2 - hy == b_bf_y && hmx - dim1 < b_bf_x)
			{
				b_box_x = dim1;
				b_box_y = dim2;
				b_box_z = dim3;
				b_bf_x = hmx - dim1;
				b_bf_y = dim2 - hy;
				b_bf_z = abs(hz - dim3);
				b_box_i = x;
			}
			else if (dim2 - hy == b_bf_y && hmx - dim1 == b_bf_x && abs(hz - dim3) < b_bf_z)
			{
				b_box_x = dim1;
				b_box_y = dim2;
				b_box_z = dim3;
				b_bf_x = hmx - dim1;
				b_bf_y = dim2 - hy;
				b_bf_z = abs(hz - dim3);
				b_box_i = x;
			}
		}
	}
}


// Find first to be packed gap in the layer edge 
void find_smallest_z(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	scrap_member = scrap_first;
	smallest_z = scrap_member;
	while (!(scrap_member->next == NULL))
	{
		if (scrap_member->next->cum_z < smallest_z->cum_z)
		{
			smallest_z = scrap_member->next;
		}
		scrap_member = scrap_member->next;
	}
	return;
}


// Check candidate boxes and the condition of the layer after finding box
void check_if_found(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	evened = 0;
	if (box_i)
	{
		current_box_i = box_i;
		current_box_x = box_x;
		current_box_y = box_y;
		current_box_z = box_z;
	}
	else
	{
		if ((b_box_i > 0) && (layer_in_layer || (!smallest_z->prev && !smallest_z->next)))
		{
			if (!layer_in_layer)
			{
				prelayer = layer_thickness;
				lilz = smallest_z->cum_z;
			}
			current_box_i = b_box_i;
			current_box_x = b_box_x;
			current_box_y = b_box_y;
			current_box_z = b_box_z;
			layer_in_layer = layer_in_layer + b_box_y - layer_thickness;
			layer_thickness = b_box_y;
		}
		else
		{
			if (!smallest_z->prev && !smallest_z->next)
			{
				layer_done = 1;
			}
			else
			{
				evened = 1;
				if (!smallest_z->prev)
				{
					trash = smallest_z->next;
					smallest_z->cum_x = smallest_z->next->cum_x;
					smallest_z->cum_z = smallest_z->next->cum_z;
					smallest_z->next = smallest_z->next->next;
					if (smallest_z->next)
					{
						smallest_z->next->prev = smallest_z;
					}
					free(trash);
				}
				else if (!smallest_z->next)
				{
					smallest_z->prev->next = NULL;
					smallest_z->prev->cum_x = smallest_z->cum_x;
					free(smallest_z);
				}
				else
				{
					if (smallest_z->prev->cum_z == smallest_z->next->cum_z)
					{
						smallest_z->prev->next = smallest_z->next->next;
						if (smallest_z->next->next)
						{
							smallest_z->next->next->prev = smallest_z->prev;
						}
						smallest_z->prev->cum_x = smallest_z->next->cum_x;
						free(smallest_z->next);
						free(smallest_z);
					}
					else
					{
						smallest_z->prev->next = smallest_z->next;
						smallest_z->next->prev = smallest_z->prev;
						if (smallest_z->prev->cum_z < smallest_z->next->cum_z)
						{
							smallest_z->prev->cum_x = smallest_z->cum_x;
						}
						free(smallest_z);
					}
				}
			}
		}
	}
	return;
}


// Check 100% condition after packing of each box
void volume_check(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	boxlist[current_box_i].is_packed = TRUE;
	boxlist[current_box_i].pack_x = current_box_x;
	boxlist[current_box_i].pack_y = current_box_y;
	boxlist[current_box_i].pack_z = current_box_z;
	packed_volume = packed_volume + boxlist[current_box_i].volume;
	packed_numbox++;
	if (packing_best)
	{
		write_visualization_data_file();
		write_boxlist_file();
	}
	else if (packed_volume == total_pallet_volume || packed_volume == total_box_volume)
	{
		packing = 0;
		hundred_percent = 1;
	}
	return;
}


// Write ".~3d" file report
void write_visualization_data_file(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	if (!unpacked)
	{
		sprintf(str_SKU, "%s", boxlist[current_box_i].SKU);
		sprintf(str_co_x, "%d", boxlist[current_box_i].co_x);
		sprintf(str_co_y, "%d", boxlist[current_box_i].co_y);
		sprintf(str_co_z, "%d", boxlist[current_box_i].co_z);
		sprintf(str_pack_x, "%d", boxlist[current_box_i].pack_x);
		sprintf(str_pack_y, "%d", boxlist[current_box_i].pack_y);
		sprintf(str_pack_z, "%d", boxlist[current_box_i].pack_z);
		sprintf(str_weight, "%d", boxlist[current_box_i].weight);
	}
	else
	{
		sprintf(str_SKU, "%s", boxlist[current_box_i].SKU);
		sprintf(str_pack_x, "%d", boxlist[current_box_i].dim1);
		sprintf(str_pack_y, "%d", boxlist[current_box_i].dim2);
		sprintf(str_pack_z, "%d", boxlist[current_box_i].dim3);
		sprintf(str_pack_w, "%d", boxlist[current_box_i].weight);
		// Add 1
	}
	if (!unpacked)
	{
		fprintf(visualizer_file, "[\"%8s\", %5s, %5s, %5s, %5s, %5s, %5s, %5s],\n ", str_SKU, str_co_x, str_co_y, str_co_z, str_pack_x, str_pack_y, str_pack_z, str_weight);
	}
	else
	{
		fprintf(report_output_file, "\"%8s\", %5s, %5s, %5s, %5s\n", str_SKU, str_pack_x, str_pack_y, str_pack_z, str_pack_w);
	}
}

void write_visualization_data_file_not_packed(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	sprintf(str_SKU, "%s", boxlist[current_box_i].SKU);
	sprintf(str_pack_x, "%d", boxlist[current_box_i].dim1);
	sprintf(str_pack_y, "%d", boxlist[current_box_i].dim2);
	sprintf(str_pack_z, "%d", boxlist[current_box_i].dim3);
	sprintf(str_pack_w, "%d", boxlist[current_box_i].weight);
	// And add 1
	fprintf(visualizer_file, "[\"%8s\", %5s, %5s, %5s, %5s, 1],\n ", str_SKU, str_pack_x, str_pack_y, str_pack_z, str_pack_w);

}


// Simple version of report (from original document) and transform found coord. sys. to entered by user
void write_boxlist_file(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	char str_x[5];
	char str_pack_st[5];
	char str_dim1[5], str_dim2[5], str_dim3[5];
	char str_co_x[5], str_co_y[5], str_co_z[5];
	char str_pack_x[5], str_pack_y[5], str_pack_z[5];

	short int x, y, z, b_x, b_y, b_z;

	// sick!
	switch (best_variant)
	{
	case 1:
		x = boxlist[current_box_i].co_x;
		y = boxlist[current_box_i].co_y;
		z = boxlist[current_box_i].co_z;
		b_x = boxlist[current_box_i].pack_x;
		b_y = boxlist[current_box_i].pack_y;
		b_z = boxlist[current_box_i].pack_z;
		break;
	case 2:
		x = boxlist[current_box_i].co_z;
		y = boxlist[current_box_i].co_y;
		z = boxlist[current_box_i].co_x;
		b_x = boxlist[current_box_i].pack_z;
		b_y = boxlist[current_box_i].pack_y;
		b_z = boxlist[current_box_i].pack_x;
		break;
	}

	sprintf(str_x, "%d", current_box_i);
	sprintf(str_pack_st, "%d", boxlist[current_box_i].is_packed);
	sprintf(str_dim1, "%d", boxlist[current_box_i].dim1);
	sprintf(str_dim2, "%d", boxlist[current_box_i].dim2);
	sprintf(str_dim3, "%d", boxlist[current_box_i].dim3);
	sprintf(str_co_x, "%d", x);
	sprintf(str_co_y, "%d", y);
	sprintf(str_co_z, "%d", z);
	sprintf(str_pack_x, "%d", b_x);
	sprintf(str_pack_y, "%d", b_y);
	sprintf(str_pack_z, "%d", b_z);

	boxlist[current_box_i].co_x = x;
	boxlist[current_box_i].co_y = y;
	boxlist[current_box_i].co_z = z;
	boxlist[current_box_i].pack_x = b_x;
	boxlist[current_box_i].pack_y = b_y;
	boxlist[current_box_i].pack_z = b_z;
	fprintf(report_output_file, "%5s%5s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n", str_x, str_pack_st, str_dim1, str_dim2, str_dim3, str_co_x, str_co_y, str_co_z, str_pack_x, str_pack_y, str_pack_z);
	return;
}


// Create all versions of report and print in console simplify result
void report_results(void)
{
#pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
	//switch (best_variant)
	//{
	//case 1:
	//	pallet_x = xx; pallet_y = yy; pallet_z = zz;
	//	break;
	//case 2:
	//	pallet_x = zz; pallet_y = yy; pallet_z = xx;
	//	break;
	//case 3:
	//	pallet_x = zz; pallet_y = xx; pallet_z = yy;
	//	break;
	//case 4:
	//	pallet_x = yy; pallet_y = xx; pallet_z = zz;
	//	break;
	//case 5:
	//	pallet_x = xx; pallet_y = zz; pallet_z = yy;
	//	break;
	//case 6:
	//	pallet_x = yy; pallet_y = zz; pallet_z = xx;
	//	break;
	//}
	

	// sick!
	switch (best_variant)
	{
	case 1:
		pallet_x = xx; pallet_y = yy; pallet_z = zz;
		break;
	case 2:
		pallet_x = zz; pallet_y = yy; pallet_z = xx;
		break;
	}


	packing_best = 1;
	if ((visualizer_file = fopen(graphout, "w")) == NULL)
	{
		printf("Cannot open file %s\n", filename);
		exit(1);
	}

	sprintf(str_p_x, "%d", pallet_x);
	sprintf(str_p_y, "%d", pallet_y);
	sprintf(str_p_z, "%d", pallet_z);

	// Some terrible json formatter, just for science (sorry for that, mvp)
	fprintf(visualizer_file, "{\n \"pallet\" : {\n \"pallet_x\" : %5s,\n \"pallet_y\" : %5s,\n \"pallet_z\" : %5s\n },\n \"boxes\" : [\n ", str_p_x, str_p_y, str_p_z);
	strcat(filename, ".out");

	if ((report_output_file = fopen(filename, "w")) == NULL)
	{
		printf("Cannot open output file %s\n", filename);
		exit(1);
	}

	packed_box_percentage = best_solution_volume * 100 / total_box_volume;
	pallet_volume_used_percentage = best_solution_volume * 100 / total_pallet_volume;
	calc_time = difftime(finish, start);

	// Simple report from original document
	fprintf(report_output_file, "ELAPSED TIME                                          : Almost %.0f sec\n", calc_time);
	fprintf(report_output_file, "TOTAL NUMBER OF ITERATIONS DONE                       : %d\n", number_of_iterations);
	fprintf(report_output_file, "BEST SOLUTION FOUND AT ITERATION                      : %d OF VARIANT: %d\n", best_iteration, best_variant);
	fprintf(report_output_file, "TOTAL NUMBER OF BOXES                                 : %d\n", all_boxes);
	fprintf(report_output_file, "PACKED NUMBER OF BOXES                                : %d\n", number_packed_boxes);
	fprintf(report_output_file, "TOTAL VOLUME OF ALL BOXES                             : %.0f\n", total_box_volume);
	fprintf(report_output_file, "PALLET VOLUME                                         : %.0f\n", total_pallet_volume);
	fprintf(report_output_file, "BEST SOLUTION'S VOLUME UTILIZATION                    : %.0f OUT OF %.0f\n", best_solution_volume, total_pallet_volume);
	fprintf(report_output_file, "PERCENTAGE OF PALLET VOLUME USED                      : %.6f %%\n", pallet_volume_used_percentage);
	fprintf(report_output_file, "PERCENTAGE OF PACKED BOXES (VOLUME)                   : %.6f %%\n", packed_box_percentage);
	fprintf(report_output_file, "WHILE PALLET ORIENTATION                              : X = %d; Y = %d; Z = %d\n", pallet_x, pallet_y, pallet_z);
	fprintf(report_output_file, "---------------------------------------------------------------------------------------------\n");
	fprintf(report_output_file, "  NO: PACKSTA DIMEN-1  DIMEN-2  DIMEN-3  COOR-X   COOR-Y   COOR-Z   PACKEDX  PACKEDY  PACKEDZ\n");
	fprintf(report_output_file, "---------------------------------------------------------------------------------------------\n");

	list_candidate_layers();
	layers[0].layer_eval = -1;
	qsort(layers, layer_listlen + 1, sizeof(struct layerlist), compute_layer_list);
	packed_volume = 0.0;
	packed_y = 0;
	packing = 1;
	layer_thickness = layers[best_iteration].layer_dim;
	remain_p_y = pallet_y;
	remain_p_z = pallet_z;

	for (x = 1; x <= all_boxes; x++)
	{
		boxlist[x].is_packed = FALSE;
	}

	do
	{
		layer_in_layer = 0;
		layer_done = 0;
		pack_layer();
		packed_y = packed_y + layer_thickness;
		remain_p_y = pallet_y - packed_y;
		if (layer_in_layer)
		{
			prepacked_y = packed_y;
			preremain_p_y = remain_p_y;
			remain_p_y = layer_thickness - prelayer;
			packed_y = packed_y - layer_thickness + prelayer;
			remain_p_z = lilz;
			layer_thickness = layer_in_layer;
			layer_done = 0;
			pack_layer();
			packed_y = prepacked_y;
			remain_p_y = preremain_p_y;
			remain_p_z = pallet_z;
		}
		find_layer(remain_p_y);
	} while (packing);

	fprintf(report_output_file, "\n\nLIST OF UNPACKED BOXES\n");
	unpacked = 1;

	for (current_box_i = 1; current_box_i <= all_boxes; current_box_i++)
	{
		if (!boxlist[current_box_i].is_packed)
		{
			write_visualization_data_file();
		}
	}

	unpacked = 0;
	fprintf(visualizer_file, "[]\n ],\n \"not_packed\" : [\n ");

	for (current_box_i = 1; current_box_i <= all_boxes; current_box_i++)
	{
		if (!boxlist[current_box_i].is_packed)
		{
			write_visualization_data_file_not_packed();
		}
	}

	unpacked = 0;
	fclose(report_output_file);
	fprintf(visualizer_file, "[]\n ]\n}");
	fclose(visualizer_file);

	// Console report
	printf("\nPACKED BOXES IN PALLET\n");
	printf("  No.    Size (3 Dim);      Corner Coords;     Axis Lenght;   Weight\n");
	for (n = 1; n <= all_boxes; n++)
	{
		if (boxlist[n].is_packed)
		{
			printf("%4d. %5d %5d %5d; %5d %5d %5d; %5d %5d %5d; %5d\n", n, boxlist[n].dim1, boxlist[n].dim2, boxlist[n].dim3, boxlist[n].co_x, boxlist[n].co_y, boxlist[n].co_z, boxlist[n].pack_x, boxlist[n].pack_y, boxlist[n].pack_z, boxlist[n].weight);
		}
	}
	printf("ELAPSED TIME                       : ~%.0f sec\n", calc_time);
	printf("TOTAL NUMBER OF BOXES              : %d\n", all_boxes);
	printf("PACKED NUMBER OF BOXES             : %d\n", number_packed_boxes);
	printf("TOTAL VOLUME OF ALL BOXES          : %.0f\n", total_box_volume);
	printf("PALLET VOLUME                      : %.0f\n", total_pallet_volume);
	printf("PERCENTAGE OF PALLET VOLUME USED   : %.6f %%\n", pallet_volume_used_percentage);
	printf("PERCENTAGE OF PACKED BOXES (VOLUME): %.6f%%\n", packed_box_percentage);
	printf("WHILE PALLET ORIENTATION           : X = %d; Y = %d; Z = %d\n", pallet_x, pallet_y, pallet_z);
}
}