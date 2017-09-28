/* C/C++ Includes */

#include <stdlib.h>

/* Arduino Library Includes */

#include "TaskAction.h"

/* Application Includes */

#include "application.h"
#include "buttons.h"
#include "led_manager.h"

/* Defines, typedefs, constants */

static const char * TOP_ROW_ORDER = "ABCDEFGH";
static const char * MIDDLE_ROW_ORDER = "FACEHDBG";
static const char * BOTTOM_ROW_ORDER = "DEAFGHCB";

static const char * BUTTON_ORDER[NUMBER_OF_ROWS] = {
	TOP_ROW_ORDER,
	MIDDLE_ROW_ORDER,
	BOTTOM_ROW_ORDER
};

static const int GAME_FINISHED = BUTTONS_PER_ROW;

static const int MAGLOCK_PIN = 3;

/* Private Variables */

static bool s_completed_combinations[BUTTONS_PER_ROW] = {false, false, false, false, false, false, false, false};

static volatile bool s_button_update_flag = false;
static volatile bool s_fake_flag = false;

/* Private Functions */

static void maglock_unlock(bool unlock)
{
	digitalWrite(MAGLOCK_PIN, unlock ? LOW : HIGH);
}

static bool all_match(unsigned char const * const c, int n)
{
	for (int i=1; i<n; i++)
	{
		if (c[0] != c[i]) { return false; }
	}
	return true;
}

static int count_true(bool * bools, int n)
{
	int count = 0;
	for (int i=0; i<n; i++)
	{
		if (bools[i]) { count++; }
	}
	return count;
}

static inline unsigned char button_to_letter(int button) { return button + 'A'; }

static int find_letter_in_order(unsigned char letter, char const * const order)
{
	for (int i = 0; i < BUTTONS_PER_ROW; i++)
	{
		if (order[i] == letter) { return i; }
	}
	return -1;
}

//static bool exactly_one_button_pressed_in_each_row()
//{
//	return (buttons_pressed_count(0) == 1) && (buttons_pressed_count(1) == 1) && (buttons_pressed_count(2) == 1);
//}

/*static int get_button_pressed_in_row(int r)
{
	for(int b=0; b<BUTTONS_PER_ROW;b++)
	{
		if (buttons_get(r, b)->pressed) { return b; }
	}
	return -1;
}

void get_button_index_pressed_in_each_row(int * button_states)
{
	for(int i=0; i<NUMBER_OF_ROWS; i++)
	{
		button_states[i] = get_last_button_pressed_in_row(i);
	}
}*/

static bool any_are_null(void * ptrs[], const int n)
{
	for (int i=0; i < n; i++)
	{
		if (!ptrs[i]) { return true; }
	}
	return false;
}

static bool array_contains(int needle, int * arr, int n)
{
	for (int i=0; i<n; i++)
	{
		if (arr[i] == needle) { return true; }
	}
	return false;
}

static bool all_are_different(int * ints, int n)
{
	for (int i=0; i<n; i++)
	{
		if (array_contains(ints[i], &ints[i+1], n-i-1)) { return false; }
	}
	return true;
}

static void order_by_row(BUTTON * buttons[3])
{
	BUTTON * copy[3] = {buttons[0], buttons[1], buttons[2]};

	for (int i; i<2; i++)
	{
		if (copy[i]->row == 0) { buttons[0] = copy[i]; continue; }
		if (copy[i]->row == 1) { buttons[1] = copy[i]; continue; }
		if (copy[i]->row == 2) { buttons[2] = copy[i]; continue; }
	}
}

static void print_buttons(BUTTON * btns[3])
{
	for (int8_t i=0; i<3; i++)
	{
		if (btns[i])
		{
			Serial.print(btns[i]->row);
			Serial.print(btns[i]->col);
		}
		else
		{
			Serial.print("XX");
		}
		Serial.print(",");
	}
}

static void signal_bad_combination()
{
	led_manager_set_blink(0, 2);
	led_manager_set_blink(1, 2);
	led_manager_set_blink(2, 2);
}

static void update_game_state(bool * completed_combinations)
{

	BUTTON * p_last_three_buttons[3];

	Serial.print("Updating state...");

	int button_columns[NUMBER_OF_ROWS];
	unsigned char button_letters[NUMBER_OF_ROWS];

//	if (!exactly_one_button_pressed_in_each_row()) { return; }

	//get_button_index_pressed_in_each_row(button_columns);

	bool last_three_buttons_are_in_different_rows;

	int last_three_button_rows[3];
	buttons_get_last_three_pressed(p_last_three_buttons);

	if (any_are_null((void**)p_last_three_buttons, 3)) { Serial.println(" nulls."); return; }

	last_three_button_rows[0] = p_last_three_buttons[0]->row;
	last_three_button_rows[1] = p_last_three_buttons[1]->row;
	last_three_button_rows[2] = p_last_three_buttons[2]->row;

	last_three_buttons_are_in_different_rows = all_are_different(last_three_button_rows, 3);

	if (!last_three_buttons_are_in_different_rows) { Serial.println(" rows diff."); return; }

	qsort(p_last_three_buttons, 3, sizeof(BUTTON*), button_compare_rows);

	Serial.print("Handling buttons ");
	print_buttons(p_last_three_buttons);
	Serial.println("");

	button_columns[0] = p_last_three_buttons[0]->col;
	button_columns[1] = p_last_three_buttons[1]->col;
	button_columns[2] = p_last_three_buttons[2]->col;
	
	for(int8_t i=0; i<NUMBER_OF_ROWS; i++)
	{
		button_letters[i] = BUTTON_ORDER[i][button_columns[i]];//find_letter_in_order(button_columns[i], BUTTON_ORDER[i]);
	}

	if (all_match(button_letters, NUMBER_OF_ROWS))
	{
		if (completed_combinations[button_letters[0] - 'A'])
		{
			Serial.print("Existing match ");
			Serial.print((char)button_letters[0]);
			Serial.println("!");	
			signal_bad_combination();
		}
		else
		{
			Serial.print("Match ");
			Serial.print((char)button_letters[0]);
			Serial.println("!");
			completed_combinations[button_letters[0] - 'A'] = true;
		}
	}
	else
	{
		Serial.println("No match.");
	}
	buttons_clear_history();
}

static void handle_game_state(bool * completed_combinations)
{
	static int8_t s_number_complete = -1;

	int8_t number_complete = count_true(completed_combinations, BUTTONS_PER_ROW);

	if (s_number_complete != number_complete)
	{
		s_number_complete = number_complete;
		Serial.print("Complete: ");
		Serial.println(s_number_complete);
		led_manager_set_completed_bars(number_complete);
	}

	maglock_unlock(number_complete == BUTTONS_PER_ROW);
}

/*static void handle_led_blink()
{
	for(int8_t r=0; r<NUMBER_OF_ROWS; r++)
	{
		for (int8_t b=0; b<BUTTONS_PER_ROW; b++)
		{
			if (buttons_get(r, b)->just_pressed)
			{
				led_manager_set_blink(r, 1);
			}	
		}
	}
}*/

static void debug_task_fn(TaskAction * this_task)
{
	(void)this_task;
	Serial.print("State: ");
	for(int8_t i=0; i < BUTTONS_PER_ROW; i++)
	{
		Serial.print(s_completed_combinations[i] ? "1" : "0");
	}
	Serial.println("");

/*	Serial.print("Last 3 buttons: ");
	for (int8_t i=0; i<3; i++)
	{
		if (p_last_three_buttons[i])
		{
			Serial.print(p_last_three_buttons[i]->row);
			Serial.print(p_last_three_buttons[i]->col);
		}
		else
		{
			Serial.print("XX");
		}
		Serial.print(",");
	}
	Serial.println("");*/
};
static TaskAction s_debug_task(debug_task_fn, 500, INFINITE_TICKS);

static void fake_next_sequence()
{
	int next_combination_index = 0;
	while(s_completed_combinations[next_combination_index])
	{
		next_combination_index++;
		if (next_combination_index == 8) { return; }
	}

	char sequence_letter = 'A' + next_combination_index;
	Serial.print("Faking sequence:"); Serial.print(sequence_letter); 
	Serial.print("(");

	uint8_t fake_buttons[3];
	for (uint8_t i = 0; i<3; i++)
	{
		fake_buttons[i] = find_letter_in_order(sequence_letter, BUTTON_ORDER[i]);
		Serial.print(fake_buttons[i]); Serial.print(',');
	}
	Serial.println(")");

	buttons_fake(fake_buttons);
}

/* Public Functions */

void setup()
{
	Serial.begin(115200);

	led_manager_setup();
	buttons_setup((bool&)s_button_update_flag);

	pinMode(MAGLOCK_PIN, OUTPUT);
	maglock_unlock(false);

}

void loop()
{
	buttons_service();
	led_manager_service();
	s_debug_task.tick();

	if (s_button_update_flag)
	{
		s_button_update_flag = false;
		update_game_state(s_completed_combinations);
		handle_game_state(s_completed_combinations);
	}

	if (s_fake_flag)
	{
		s_fake_flag = false;
		fake_next_sequence();
	}
}

void serialEvent()
{
	while (Serial.available())
	{
		char c = Serial.read();
		if (c == 'f')
		{
			s_fake_flag = true;
		}
	}
}