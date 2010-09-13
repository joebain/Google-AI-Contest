#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <sys/time.h>
#include <math.h>
#include "PlanetWars.h"

std::ofstream outfile;
const PlanetWars* planetwars;
double centre_x = 0, centre_y = 0;
int my_ships = 0, their_ships = 0, neutral_ships = 0, total_ships = 0;
double local_importance = 0.2;
double foreign_importance = 0.5;
double neutral_importance = 1.0;
int min_ships = 50;
timeval start, last;

#define MINE 1
#define THEIRS 2
#define NEUTRAL 0

void Log(std::string text) {
	outfile << text << std::endl;
	outfile.flush();
}

void Time() {
	timeval now;
	gettimeofday(&now, NULL);
	outfile << "time: " << (now.tv_usec - start.tv_usec)/1000000.0 << std::endl;
	last = now;
}

void ResetTime() {
	gettimeofday(&start, NULL);
}

bool PlanetCompare(Planet* one, Planet* two) {
	return one->desire > two->desire;
}

void SortPlanets(std::vector<Planet*>& planets) {
	std::sort(planets.begin(), planets.end(), PlanetCompare);
}

void SetDesires(std::vector<Planet>& planets) {
	for (std::vector<Planet>::iterator planet = planets.begin() ; planet != planets.end() ; ++planet) {
		planet->desire = (1.0 - (planet->NumShips() / (double)total_ships));
		double distance = sqrt( pow(planet->Y() - centre_y, 2) + pow(planet->X() - centre_x, 2));
		if (distance > 0.1)
			planet->desire /= distance;
		if (planet->Owner() == MINE) {
			planet->desire *= local_importance;
			planet->ships_wanted = 0;
		} else if (planet->Owner() >= THEIRS) {
			planet->desire *= foreign_importance;
			planet->ships_wanted = planet->NumShips()+1;
		} else {
			planet->desire *= neutral_importance;
			planet->ships_wanted = planet->NumShips()+1;
		}
	}
}

void FindCentre() {
	double running_x = 0, running_y = 0;
	std::vector<Planet> planets = planetwars->MyPlanets();
	for (std::vector<Planet>::iterator planet = planets.begin() ; planet != planets.end() ; ++planet) {
		running_x += planet->X() * planet->NumShips();
		running_y += planet->Y() * planet->NumShips();
	}
	centre_x = running_x / my_ships;
	centre_y = running_y / my_ships;
}

void GetCounts() {
	my_ships = 0;
	their_ships = 0;
	neutral_ships = 0;
	total_ships = 0;
	int i = 0;
	std::vector<Planet> planets = planetwars->Planets();
	for (std::vector<Planet>::iterator planet = planets.begin() ; planet != planets.end() ; ++planet) {
		if (planet->Owner() == MINE) {
			my_ships += planet->NumShips();
		}
		else if (planet->Owner() == NEUTRAL) {
			neutral_ships += planet->NumShips();
		}
		else if (planet->Owner() >= THEIRS) {
			their_ships += planet->NumShips();
		}
		++i;
	}
	total_ships = my_ships + their_ships + neutral_ships;
	outfile << "ships: " << my_ships << ", " << their_ships << std::endl;
}

void LogPlanets(std::vector<Planet*>& planets) {
	for (std::vector<Planet*>::iterator planet = planets.begin() ; 
			planet != planets.end() ; ++planet) {
		outfile << (*planet)->PlanetID() <<
			": des- " << (*planet)->desire <<
			" pop- " << (*planet)->NumShips() <<
			" req- " << (*planet)->ships_wanted << 
			std::endl;
	}
}

void Order(int source, int target, int ships) {
	outfile << "order: " << source << "=>" << target << "(" << ships << ")" << std::endl;
	if (ships <= 5) return;
	planetwars->IssueOrder(source, target, ships);
}

std::map<int, Planet*> PlanetMap(std::vector<Planet>& planets) {
	std::map<int, Planet*> planet_map;
	for (std::vector<Planet>::iterator planet = planets.begin() ;
			planet != planets.end(); ++planet) {
		planet_map[planet->PlanetID()] = &(*planet);
	}
	return planet_map;
}

std::vector<Planet*> ToPointers(std::vector<Planet>& planets) {
	std::vector<Planet*> p_planets;
	for (std::vector<Planet>::iterator planet = planets.begin() ;
			planet != planets.end(); ++planet) {
		p_planets.push_back(&(*planet));
	}
	return p_planets;
}

std::vector<Planet*> MyPlanets(std::vector<Planet*>& planets) {
	std::vector<Planet*> my_planets;
	for (std::vector<Planet*>::iterator planet = planets.begin() ;
			planet != planets.end(); ++planet) {
		if ((*planet)->Owner() == MINE) {
			my_planets.push_back(*planet);
		}
	}
	return my_planets;
}

void AdjustForFleets(std::map<int, Planet*> planets) {
	std::vector<Fleet> fleets = planetwars->MyFleets();
	for (std::vector<Fleet>::iterator fleet = fleets.begin() ;
			fleet != fleets.end() ; ++fleet) {

		planets[fleet->DestinationPlanet()]->ships_wanted -= fleet->NumShips();
	}
}

//FUCK THEM UP!!!
void DoTurn(const PlanetWars& pw) {

	Log("\n\nmaking a move");
	Time();
	planetwars = &pw;
	std::vector<Planet> canonical_planets = pw.Planets();
	std::vector<Planet*> planets = ToPointers(canonical_planets);

	Log("counts");
	Time();
	GetCounts();

	Log("centre");
	Time();
	FindCentre();

	Log("get desires");
	Time();
	SetDesires(canonical_planets);

	Log("sorting");
	Time();
	SortPlanets(planets);
	std::vector<Planet*> myplanets = MyPlanets(planets);
	Log("logging");
	Time();
	Log("\nMine:");
	LogPlanets(myplanets);
	Log("\nAll:");
	LogPlanets(planets);

	Log("\nadjusting for fleets");
	Time();
	AdjustForFleets(PlanetMap(canonical_planets));
	Log("\nadjusted:");
	LogPlanets(planets);

	Log("\nmoves:");
	Time();

	std::vector<Planet*>::reverse_iterator source_planet = myplanets.rbegin();
	std::vector<Planet*>::iterator target_planet = planets.begin();

	int orders = 0;

	while (source_planet < myplanets.rend() &&
			target_planet < planets.end() &&
			(*source_planet)->desire < (*target_planet)->desire) {

		outfile
			<< "sp " << (*source_planet)->PlanetID() << " : " << (*source_planet)->desire 
			<< " tp " << (*target_planet)->PlanetID() << " : " << (*target_planet)->desire
			<< std::endl;
		Time();

		if ((*source_planet)->NumShips() < min_ships) {
			++source_planet;
			continue;
		}

		if ((*target_planet)->Owner() == MINE) {
			Log("reinforcements");
			if ((*source_planet)->NumShips() > (*target_planet)->NumShips()) {
				int ships_to_send = ((*source_planet)->NumShips() - (*target_planet)->NumShips())/2;
				Order((*source_planet)->PlanetID(), (*target_planet)->PlanetID(),ships_to_send);
			}
			++target_planet;
			++source_planet;
		} else {
			int ships_required = (*target_planet)->ships_wanted + pw.Distance((*source_planet)->PlanetID(), (*target_planet)->PlanetID()) * (*target_planet)->GrowthRate() + (*target_planet)->ships_wanted;
			if ((*source_planet)->NumShips()-min_ships > ships_required) {
				Log("sending all ships needed");
				Order((*source_planet)->PlanetID(), (*target_planet)->PlanetID(),ships_required);
				(*source_planet)->RemoveShips(ships_required);
				++target_planet;
			} else {
				Log("sending some ships");
				int ships_to_send = (*source_planet)->NumShips()-min_ships;
				Order((*source_planet)->PlanetID(), (*target_planet)->PlanetID(),ships_to_send);
				Log("dec ships tp send");
				(*target_planet)->ships_wanted -= ships_to_send;
				Log("++iter");
				++source_planet;
				Log("done");
			}
		}
		Log("next");
		++orders;
	}
	outfile << "orders: " << orders << std::endl;
	Log("\nall done\n");
	Time();
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	outfile.open ("output.log");
	outfile << "lOG!!" << std::endl;
	outfile.flush();

	gettimeofday(&start, NULL);
	gettimeofday(&last, NULL);

	std::string current_line;
	std::string map_data;
	while (true) {
		int c = std::cin.get();
		current_line += (char)c;
		if (c == '\n') {
			if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {

				ResetTime();
				PlanetWars pw(map_data);
				map_data = "";
				DoTurn(pw);
	pw.FinishTurn();
			} else {
				map_data += current_line;
			}
			current_line = "";
		}
	}
	outfile.close();
	return 0;
}
