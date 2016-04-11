#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define DICE_PER_PLAYER 5
#define NUMBER_OF_PLAYERS 4

typedef struct _player{
	char* name;
	int numdice;
	int dice[DICE_PER_PLAYER];
	int isuser;
	struct _player* prev; //this is a doubly linked list too
	struct _player* next;
}player;

typedef struct _gamestate{
	int numplayers;
	player* currentplayer;
	int currentbet[2];
	int numdiceleft;
	int inprogress;
}gamestate;

void myprint(int print, const char* format, ...) {
	if (print) {
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		getch();
	}
}

void printDice(int num_dice, int* dice) {
	char top[] = { 218, 196, 196, 196, 196, 196, 191, 0 };
	char bot[] = { 192, 196, 196, 196, 196, 196, 217, 0 };
	char mid_none[] = { 179, 32, 32 , 32, 32 ,32, 179, 0 };
	char mid_l[] = { 179, 79, 32, 32, 32, 32, 179, 0 };
	char mid_m[] = { 179, 32, 32, 79, 32, 32, 179, 0 };
	char mid_r[] = { 179, 32, 32, 32, 32, 79, 179, 0 };
	char mid_lr[] = { 179, 79, 32, 32, 32, 79, 179, 0 };

	int i;
	for (i = 0; i < num_dice; i++) {
		printf("%s", top);
	}
	printf("\n");
	for (i = 0; i < num_dice; i++) {
		switch (dice[i]) {
		case 2:
		case 3:
			printf("%s", mid_l);
			break;
		case 4:
		case 5:
		case 6:
			printf("%s", mid_lr);
			break;
		default:
			printf("%s", mid_none);
		}
	}
	printf("\n");
	for (i = 0; i < num_dice; i++) {
		switch (dice[i]) {
		case 1:
		case 3:
		case 5:
			printf("%s", mid_m);
			break;
		case 6:
			printf("%s", mid_lr);
			break;
		default:
			printf("%s", mid_none);
		}
	}
	printf("\n");
	for (i = 0; i < num_dice; i++) {
		switch (dice[i]) {
		case 2:
		case 3:
			printf("%s", mid_r);
			break;
		case 4:
		case 5:
		case 6:
			printf("%s", mid_lr);
			break;
		default:
			printf("%s", mid_none);
		}
	}
	printf("\n");
	for (i = 0; i < num_dice; i++) {
		printf("%s", bot);
	}
	printf("\n");
}

player* createPlayer(int num, int is_ai) {
	player* ret = (player*)malloc(sizeof(player));
	char* name = (char*)malloc(16 * sizeof(char));
	ret->isuser = !is_ai;
	if (ret->isuser)
		sprintf(name, "User");
	else {
		sprintf(name, "Computer %d", num);
	}
	ret->name = name;
	ret->prev = NULL;
	ret->next = NULL;
	ret->numdice = DICE_PER_PLAYER;
	int i;
	for (i = 0; i < DICE_PER_PLAYER; i++) {
		ret->dice[i] = 0;
	}
	return ret;
}

void destroyPlayer(player* p) {
	if (p == NULL)
		return;
	free(p->name);
	free(p);
}

int resetBet(gamestate* game) {
	if (game == NULL)
		return -1;
	game->currentbet[0] = 0;
	game->currentbet[1] = 0;
	return 0;
}

int getNumOf(player* cur, int die) {
	int ret = 0;
	int i;
	for (i = 0; i < cur->numdice; i++) {
		if (cur->dice[i] == die)
			ret++;
	}
	return ret;
}

long long nCr(int n, int r) {
	long long rfac = 1;
	long long n_rfac = 1;
	int i;
	for (i = 1; i <= r; i++) {
		rfac *= i;
	}
	for (i = n; i > n - r; i--)
		n_rfac *= i;
	return n_rfac / rfac;
}

/* Calculates the probablility of at least num successes in total, assuming p = 1/6 */
float sumBinomialDist(int num, int total) {
	int i;
	float prob = 0;

	if (num > total)
		return 0.0;
	if (num <= 0)
		return 1.0;

	for (i = 0; i < num; i++) {
		prob += nCr(total, i) * pow(1.0 / 6, i) * pow(5.0 / 6, total - i);
	}
	return 1 - prob;
}

float calculateOdds(gamestate* game) {
	if (game->currentbet[0] == 0) {
		return 1.0;
	}
	player* cur = game->currentplayer;
	int num = game->currentbet[0];
	int die = game->currentbet[1];
	int num_other_dice = (game->numdiceleft) - (cur->numdice);

	num -= getNumOf(cur, die);
	if (game->numplayers > 2) {
		num--;
	}
	
	if (num <= 0)
		return 1.0; //player has more dice than the bet, guaranteed correct bet

	//calculate the odds that there are {num} {die} in {num_other_dice}
	return sumBinomialDist(num, num_other_dice);
}

int eliminatePlayers(gamestate* game) {
	if (game == NULL) 
		return -1;
	
	//go through all players and eliminate any with 0 dice left
	int num = game->numplayers;
	int i;
	player* cur = game->currentplayer;
	player* tmp;
	for (i = 0; i < num; i++){
		tmp = cur->next;
		if (cur->numdice <= 0) {
			game->numplayers--;
			cur->prev->next = cur->next;
			cur->next->prev = cur->prev;
			destroyPlayer(cur);
		}
		cur = tmp;
	}
	if (game->numplayers == 1)
		game->inprogress = FALSE;

	return 0;
}

void rerollPlayer(player* p) {

	int i;
	for (i = 0; i < p->numdice; i++) {
		p->dice[i] = rand() % 6 + 1;
	}
}
int reroll(gamestate* game) {
	player* cur = game->currentplayer;
	int i;
	for (i = 0; i < game->numplayers; i++) {
		rerollPlayer(cur);
		cur = cur->next;
	}
	return 0;
}

int callBet(gamestate* game) {
	if (game == NULL)
		return -1;

	//check to see if the bet was accurate
	player* cur = game->currentplayer;
	int num = 0; //number of dice that was bet on

	myprint(TRUE, "%s called out the bet of %d %d's!\n", game->currentplayer->name, game->currentbet[0], game->currentbet[1]);
	//count number of dice on table that was bet on
	do {
		num += getNumOf(cur, game->currentbet[1]);
		myprint(TRUE, "%s's dice:\n", cur->name);
		printDice(cur->numdice, cur->dice);
		cur = cur->next;
	} while (cur != game->currentplayer);

	if (num < game->currentbet[0]) { //bet was wrong
		game->currentplayer->prev->numdice--;
		myprint(TRUE, "The bet was wrong! %s loses a die!\n", game->currentplayer->prev->name);
	}
	else { //bet was right
		game->currentplayer->numdice--;
		myprint(TRUE, "The bet was correct! %s loses a die!\n", game->currentplayer->name);
		game->currentplayer = game->currentplayer->prev; //winner is new current player
	}
	game->numdiceleft--;

	eliminatePlayers(game);
	reroll(game);
	resetBet(game);
	return 0;
}

int callExact(gamestate* game) {
	if (game == NULL)
		return -1;

	//check to see if the bet was accurate
	player* cur = game->currentplayer;
	int num = 0; //number of dice that was bet on

	myprint(TRUE, "%s called the bet of %d %d's exact!\n", game->currentplayer->name, game->currentbet[0], game->currentbet[1]);
	//count number of dice on table that was bet on
	do {
		num += getNumOf(cur, game->currentbet[1]);
		myprint(TRUE, "%s's dice:\n", cur->name);
		printDice(cur->numdice, cur->dice);
		cur = cur->next;
	} while (cur != game->currentplayer);

	if (num == game->currentbet[0]) { //bet was wrong
		game->currentplayer->prev->numdice--;
		myprint(TRUE, "The bet was exact! %s loses a die!\n", game->currentplayer->prev->name);
	}
	else { //bet was right
		game->currentplayer->numdice--;
		myprint(TRUE, "The bet wasn't exact! %s loses a die!\n", game->currentplayer->name);
		game->currentplayer = game->currentplayer->prev; //winner is new current player
	}
	game->numdiceleft--;

	eliminatePlayers(game);
	reroll(game);
	resetBet(game);
	return 0;
}

int makeInitialBet(gamestate* game) {
	if (game == NULL)
		return -1;

	//figure out best bet
	int bet[2];
	int i;
	bet[0] = getNumOf(game->currentplayer, 6);
	bet[1] = 6;
	for (i = 5; i >= 1; i--) {
		//if there's more of a lower dice, go with that bet
		if (getNumOf(game->currentplayer, i) > bet[0]) {
			bet[0] = getNumOf(game->currentplayer, i);
			bet[1] = i;
		}
	}
	//add extra randomness by slightly altering the bet if there's more than 2 players
	bet[0] += rand() % (game->numplayers - 1);

	if (rand() % 5 == 0) { //20% chance to bet on random number
		bet[1] = rand() % 6 + 1;
	}
	game->currentbet[0] = bet[0];
	game->currentbet[1] = bet[1];

	myprint(TRUE, "%s raised the bet to %d %d's\n", game->currentplayer->name, game->currentbet[0], game->currentbet[1]);
	return 0;
}

int raiseBet(gamestate* game) {
	if (game == NULL)
		return -1;

	//make initial bet if bet was just reset
	if (game->currentbet[0] == 0) 
		return makeInitialBet(game);

	//calculate odds on all good bets
	float bestprob = 0.0;
	int bet[] = { 0,0 };
	int cur[] = { 0,0 };
	int i = 0;
	int num_other_dice = game->numdiceleft - game->currentplayer->numdice;

	cur[0] = game->currentbet[0];
	cur[1] = game->currentbet[1];

	//check the next 6 bets, use best
	for (i = 0; i < 6; i++) {
		//get the next bet
		cur[1]++;
		if (cur[1] > 6) {
			cur[0]++;
			cur[1] = 1;
		}

		//set the bet if it's better than the current best bet
		if (sumBinomialDist(cur[0] - getNumOf(game->currentplayer, cur[1]), num_other_dice) > bestprob) {
			bestprob = sumBinomialDist(cur[0] - getNumOf(game->currentplayer, cur[1]), num_other_dice);
			bet[0] = cur[0];
			bet[1] = cur[1];
		}
	}

	if (bestprob < 0.3)
		return 1;
	if (rand() % 5 > 0) { //80% chance to bet on best bet, 20% to just raise old bet
		game->currentbet[0] = bet[0];
		game->currentbet[1] = bet[1];
	}
	else {
		game->currentbet[0]++;
	}

	myprint(TRUE, "%s raised the bet to %d %d's\n", game->currentplayer->name, game->currentbet[0], game->currentbet[1]);
	
	return 0;
}

/* Gets what the user wants to do.  Returns 'r' for raise, 'c' for call, 'e' for exact*/
char getUserDecision(gamestate* game) {
	char str[80];
	if (game->currentbet[0] == 0)
		return 'r'; //Gotta raise if there isn't a bet yet

	printf("\nThe current bet is %d %d's\n", game->currentbet[0], game->currentbet[1]);
	printf("(r)aise bet, (c)all bet, call (e)xact: ");
	scanf("%s", str);

	while (tolower(str[0]) != 'r' && tolower(str[0]) != 'c' && tolower(str[0]) != 'e') {
		printf("\nThat wasn't a valid input.\n(r)aise bet, (c)all bet, call (e)xact: ");
		scanf("%s", str);
	}
	return str[0];
}

int raiseBetUser(gamestate* game) {
	int bet[2];

	printf("\nEnter your bet (format: %%d %%d): ");
	scanf("%d %d", &bet[0], &bet[1]);

	while ((bet[0] < game->currentbet[0]) || 
		(bet[0] == game->currentbet[0] && bet[1] <= game->currentbet[1]) ||
		(bet[1] > 6 || bet[1] < 1) || 
		(bet[0] < 1))
	{

		printf("That bet isn't valid\n");
		printf("\nEnter your bet (format: %%d %%d): ");
		scanf("%d %d", &bet[0], &bet[1]);
	}

	game->currentbet[0] = bet[0];
	game->currentbet[1] = bet[1];
	return 0;
}

int playUser(gamestate* game) {
	player* cur = game->currentplayer;

	//print dice to user
	printDice(cur->numdice, cur->dice);

	//get user input on if they want to call or raise bet
	char decision = getUserDecision(game);

	if (decision == 'r') {
		//let the user raise the bet
		raiseBetUser(game);
		game->currentplayer = game->currentplayer->next;
	}
	else if (decision == 'c') {
		callBet(game);		
	}
	else if (decision == 'e') {
		callExact(game);
	}
	return 0;
}

int playTurn(gamestate* game) {
	if (game == NULL)
		return -1;

	if (game->currentplayer->isuser) {
		//play user here
		playUser(game);
	}
	else {
		//check to see if we want to raise bet
		if (calculateOdds(game) < 0.4){
			//call out bet
			callBet(game);
		}
		else {
			//raise bet
			if (raiseBet(game) == 1)
				callExact(game);
			else
				game->currentplayer = game->currentplayer->next;
		}
	}
	return 0;
}

int initializeGame(gamestate* game, int numplayers, int ai_only) {
	if (game == NULL) {
		return -1;
	}
	if (numplayers < 2)
		return -1;

	game->numplayers = numplayers;
	game->numdiceleft = numplayers * DICE_PER_PLAYER;
	game->inprogress = TRUE;
	resetBet(game);

	player* cur, *prev, *first;
	//first player is user if user is playing
	first = createPlayer(0, ai_only);
	prev = first;
	
	int i;
	for (i = 0; i < (numplayers - (1 - ai_only)); i++) {
		cur = createPlayer(i+1, TRUE);
		prev->next = cur;
		cur->prev = prev;
		prev = cur;
	}
	cur->next = first;
	first->prev = cur;

	game->currentplayer = first;
	reroll(game);
	
	return 0;
}

int main(int argc, char** argv) {
	gamestate game;

	srand(time(NULL));
	initializeGame(&game, NUMBER_OF_PLAYERS, FALSE);
	while (game.inprogress) {
		playTurn(&game);
	}
}