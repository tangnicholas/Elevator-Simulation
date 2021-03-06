// Names: Sabrina Ly & Raymond Chu
// Student Numbers: 16997158 & 23248157
#include "C:\RTExamples\rt.h"
#include <ElevatorData.h>
#include <ElevatorStatus.h>

//Function Headers
int check_empty_array();
void clear_floor_array();
void close_door();
void open_door();
void stop_elevator(int);
void update_status();
void EV2_DW_RESET();
void EV2_DW_SIGNAL();
void EV2_UP_RESET();
void EV2_UP_SIGNAL();

ElevatorStatus Elevator2Status("Elevator2");
elevator_status status;
UINT Message;
int elevator_floor = 0;
int elevator_direction = 1; // 1 = up, 0 = down
int target_floor = 0;
int end_sim = 0;
int fault = 0;
int destroy = 0;
int changed_floor = 0;
int req_direction = 0;

UINT __stdcall Elevator2Move(void *args)
{
	r1.Wait();
	while (!destroy)
	{

		/*********  ELEVATOR GOING UP  **********/
		while (elevator_floor < target_floor && !destroy)
		{
			elevator_direction = UP;
			if (EV2UP_array[elevator_floor].stop)
			{
				stop_elevator(UP);
			}
			Sleep(MOVE_DELAY);
			elevator_floor++;
			changed_floor = 1;
			update_status();
			changed_floor = 0;
		}
		/*********  ELEVATOR GOING DOWN  **********/
		while (elevator_floor > target_floor && !destroy)
		{
			elevator_direction = DOWN;
			if (EV2DOWN_array[elevator_floor].stop)
			{
				stop_elevator(DOWN);
			}
			Sleep(MOVE_DELAY);
			elevator_floor--;
			changed_floor = 1;
			update_status();
			changed_floor = 0;
		}
		/*********  ELEVATOR REACHED TARGET FLOOR  **********/
		if (elevator_floor == target_floor)
		{
			if (end_sim)
			{
				if (elevator_floor == 0)
				{
					cout << "Reached here" << endl;
					open_door();
					EV2SimFinished.Signal();
					EV2SimFinished.Signal();
				}
			}
			else if (check_empty_array() == 0)
			{
				while (check_empty_array() == 0)
				{
					// do nothing
				}
			}
			else if (elevator_direction == UP)
			{
				//stop elevator
				stop_elevator(UP);
			}
			else if (elevator_direction == DOWN)
			{
				//stop elevator
				stop_elevator(DOWN);
			}
			update_status();
		}
	}

	return 0;
}

int main()
{
	CThread t1(Elevator2Move, ACTIVE, NULL);
	r1.Wait();

	int stopped_flag = 0;

	// initialize status: floor 0, going up, target floor 0
	update_status();

	/**================================================== *
	 * ==========  Listen for Commands  ========== *
	 * ================================================== */

	while (1)
	{
		Message = Elevator2Mailbox.GetMessage();
		int command_type = Message / 10;
		int req_floor = Message % 10;
		req_direction = command_type;

		/**================================================== *
		 * ==========  Section Populate Elevator Array  ========== *
		 * ================================================== */
		cout << "Message Received: " << Message << endl;
		if (Message == E2_FAULT)
		{
			clear_floor_array();
			fault = 1;
			target_floor = elevator_floor; // doing nothing in other thread
										   // next message has to be clearing fault, dealt with in IO
		}
		else if (Message == E2_CLEAR)
		{
			fault = 0;
		}
		else if (Message == END_SIM)
		{
			target_floor = 0;
			end_sim = 1;
			fault = 1;
			cout << "Received END_SIM" << endl;
			clear_floor_array();
			EV2DOWN_array[0].stop = 1;
			update_status();
			break;
			// TODO: open doors
		}
		else if (Message == START_PASSENGERS)
		{
			clear_floor_array();
			target_floor = elevator_floor;
			cout << "target floor setting to " << elevator_floor << endl;
			update_status();
		}
		else if (Message == END_PASSENGERS)
		{
			clear_floor_array();
			target_floor = elevator_floor;
			update_status();
		}
		//if passenger is inside
		else if (command_type == INSIDE)
		{
			cout << "INSIDE" << endl;
			if (elevator_direction == UP)
			{
				cout << "INSIDE UP" << endl;
				EV2UP_array[req_floor].stop = 1;
				EV2UP_array[req_floor].passenger_inside++;
			}
			else if (elevator_direction == DOWN)
			{
				cout << "INSIDE DOWN" << endl;
				EV2DOWN_array[req_floor].stop = 1;
				EV2DOWN_array[req_floor].passenger_inside++;
			}
		}
		else if (elevator_floor == target_floor)
		{
			cout << "Target Floor" << endl;
			if (command_type == INSIDE && req_floor > elevator_floor)
			{
				cout << "Target Floor UP" << endl;
				EV2UP_array[req_floor].stop = 1;
				EV2UP_array[req_floor].passenger_inside++;
				target_floor = req_floor;
			}
			else if (command_type == INSIDE && req_floor < elevator_floor)
			{
				cout << "Target Floor DOWN" << endl;
				EV2DOWN_array[req_floor].stop = 1;
				EV2DOWN_array[req_floor].passenger_inside++;
				target_floor = req_floor;
			}
			else if (req_floor > elevator_floor)
			{
				EV2UP_array[req_floor].stop = 1;
				EV2UP_array[req_floor].passenger_outside++;
				target_floor = req_floor;
			}
			else if (req_floor < elevator_floor)
			{
				EV2DOWN_array[req_floor].stop = 1;
				EV2DOWN_array[req_floor].passenger_outside++;
				target_floor = req_floor;
			}
			else if (req_floor == elevator_floor)
			{
				// does it matter which array we populate? look @ other thread
				elevator_direction = UP;
				EV2UP_array[req_floor].stop = 1;
				EV2UP_array[req_floor].passenger_outside++;
				target_floor = req_floor;
			}
		}
		//if passenger is outside and requesting to go up
		else if (command_type == OUT_UP)
		{
			EV2UP_array[req_floor].stop = 1;
			EV2UP_array[req_floor].passenger_outside++;
		}
		//if passenger is outside and requesting to go down
		else if (command_type == OUT_DOWN)
		{
			EV2DOWN_array[req_floor].stop = 1;
			EV2DOWN_array[req_floor].passenger_outside++;
		}
		// if message is from inside elevator (0-9) and the request exceeds the current targeted floor, set as new targeted floor
		if ((Message < target_floor && elevator_direction == DOWN) || (Message <= 9 && Message > target_floor && elevator_direction == UP))
		{
			target_floor = Message;
		}
		update_status();
	}
	/* =======  End of Listen for Commands  ======= */
	EV2SimFinished.Wait();

	cout << "End of Simulation" << endl;
	destroy = 1;
	//t1.~CThread();
	//t1.WaitForThread();

	cout << "Waiting for r2" << endl;
	r2.Wait();
	return 0;
}

void stop_elevator(int elevator_direction)
{
	if (elevator_direction == UP)
	{
		open_door();
		EV2_UP_SIGNAL();
		Sleep(DOOR_DELAY);
		EV2_UP_RESET();
		close_door();
		//Update counters
		EV_passenger_count = EV_passenger_count - EV2UP_array[elevator_floor].passenger_inside + EV2UP_array[elevator_floor].passenger_outside;
		outside_issued_count -= EV2UP_array[elevator_floor].passenger_outside;
		//clear floor struct
		EV2UP_array[elevator_floor].stop = 0;
		EV2UP_array[elevator_floor].passenger_inside = 0;
		EV2UP_array[elevator_floor].passenger_outside = 0;
		update_status();
	}
	else if (elevator_direction == DOWN)
	{
		open_door();
		EV2_DW_SIGNAL();
		Sleep(DOOR_DELAY);
		EV2_DW_RESET();
		close_door();
		//Update counters
		EV_passenger_count = EV_passenger_count - EV2DOWN_array[elevator_floor].passenger_inside + EV2DOWN_array[elevator_floor].passenger_outside;
		outside_issued_count -= EV2DOWN_array[elevator_floor].passenger_outside;
		//clear floor struct
		EV2DOWN_array[elevator_floor].stop = 0;
		EV2DOWN_array[elevator_floor].passenger_inside = 0;
		EV2DOWN_array[elevator_floor].passenger_outside = 0;
		update_status();
	}
}

void open_door()
{
	door1 = 1;
	update_status();
	cursor.Wait();
	cursor.Signal();
}

void close_door()
{
	door1 = 0;
	update_status();
	cursor.Wait();
	cursor.Signal();
}

void update_status()
{
	UP_struct UP_array;
	DOWN_struct DOWN_array;

	UP_array.s0 = EV2UP_array[0];
	UP_array.s1 = EV2UP_array[1];
	UP_array.s2 = EV2UP_array[2];
	UP_array.s3 = EV2UP_array[3];
	UP_array.s4 = EV2UP_array[4];
	UP_array.s5 = EV2UP_array[5];
	UP_array.s6 = EV2UP_array[6];
	UP_array.s7 = EV2UP_array[7];
	UP_array.s8 = EV2UP_array[8];
	UP_array.s9 = EV2UP_array[9];

	DOWN_array.s0 = EV2DOWN_array[0];
	DOWN_array.s1 = EV2DOWN_array[1];
	DOWN_array.s2 = EV2DOWN_array[2];
	DOWN_array.s3 = EV2DOWN_array[3];
	DOWN_array.s4 = EV2DOWN_array[4];
	DOWN_array.s5 = EV2DOWN_array[5];
	DOWN_array.s6 = EV2DOWN_array[6];
	DOWN_array.s7 = EV2DOWN_array[7];
	DOWN_array.s8 = EV2DOWN_array[8];
	DOWN_array.s9 = EV2DOWN_array[9];

	status = {elevator_floor, elevator_direction, target_floor, EV_passenger_count, door1, fault, changed_floor, req_direction, UP_array, DOWN_array};
	Elevator2Status.Update_Status(status);
}

int check_empty_array()
{
	int stop = 0;
	for (int i = 0; i < NUM_FLOORS; i++)
	{
		if (EV2UP_array[i].stop || EV2DOWN_array[i].stop)
		{
			stop = 1;
			break;
		}
	}
	return stop;
}

void clear_floor_array()
{
	for (int i = 0; i < NUM_FLOORS; i++)
	{
		EV2UP_array[i].stop = 0;
		EV2UP_array[i].passenger_inside = 0;
		EV2UP_array[i].passenger_outside = 0;
		EV2DOWN_array[i].stop = 0;
		EV2DOWN_array[i].passenger_inside = 0;
		EV2DOWN_array[i].passenger_outside = 0;
	}
	// if fault or changing mode, clear passengers from inside
	EV_passenger_count = 0;
	update_status();
}

void EV2_UP_SIGNAL()
{
	if (elevator_floor == 0)
		EV2_UP0.Signal();
	else if (elevator_floor == 1)
		EV2_UP1.Signal();
	else if (elevator_floor == 2)
		EV2_UP2.Signal();
	else if (elevator_floor == 3)
		EV2_UP3.Signal();
	else if (elevator_floor == 4)
		EV2_UP4.Signal();
	else if (elevator_floor == 5)
		EV2_UP5.Signal();
	else if (elevator_floor == 6)
		EV2_UP6.Signal();
	else if (elevator_floor == 7)
		EV2_UP7.Signal();
	else if (elevator_floor == 8)
		EV2_UP8.Signal();
	else if (elevator_floor == 9)
		EV2_UP9.Signal();
}

void EV2_DW_SIGNAL()
{
	if (elevator_floor == 0)
		EV2_DW0.Signal();
	else if (elevator_floor == 1)
		EV2_DW1.Signal();
	else if (elevator_floor == 2)
		EV2_DW2.Signal();
	else if (elevator_floor == 3)
		EV2_DW3.Signal();
	else if (elevator_floor == 4)
		EV2_DW4.Signal();
	else if (elevator_floor == 5)
		EV2_DW5.Signal();
	else if (elevator_floor == 6)
		EV2_DW6.Signal();
	else if (elevator_floor == 7)
		EV2_DW7.Signal();
	else if (elevator_floor == 8)
		EV2_DW8.Signal();
	else if (elevator_floor == 9)
		EV2_DW9.Signal();
}

void EV2_UP_RESET()
{
	if (elevator_floor == 0)
		EV2_UP0.Reset();
	else if (elevator_floor == 1)
		EV2_UP1.Reset();
	else if (elevator_floor == 2)
		EV2_UP2.Reset();
	else if (elevator_floor == 3)
		EV2_UP3.Reset();
	else if (elevator_floor == 4)
		EV2_UP4.Reset();
	else if (elevator_floor == 5)
		EV2_UP5.Reset();
	else if (elevator_floor == 6)
		EV2_UP6.Reset();
	else if (elevator_floor == 7)
		EV2_UP7.Reset();
	else if (elevator_floor == 8)
		EV2_UP8.Reset();
	else if (elevator_floor == 9)
		EV2_UP9.Reset();
}

void EV2_DW_RESET()
{
	if (elevator_floor == 0)
		EV2_DW0.Reset();
	else if (elevator_floor == 1)
		EV2_DW1.Reset();
	else if (elevator_floor == 2)
		EV2_DW2.Reset();
	else if (elevator_floor == 3)
		EV2_DW3.Reset();
	else if (elevator_floor == 4)
		EV2_DW4.Reset();
	else if (elevator_floor == 5)
		EV2_DW5.Reset();
	else if (elevator_floor == 6)
		EV2_DW6.Reset();
	else if (elevator_floor == 7)
		EV2_DW7.Reset();
	else if (elevator_floor == 8)
		EV2_DW8.Reset();
	else if (elevator_floor == 9)
		EV2_DW9.Reset();
}