#include <cmath>
#include <iostream>
#include <sqlite3.h>
#include <vector>

#define K_FACTOR 30
#define NUM_TEAMS 30
#define BASE_RATING 1500.0

//ORM
struct Game {
    int id;
    int home_team_id;
    int home_team_runs;
    int away_team_id;
    int away_team_runs;
    int year;
    double epochtime;
};


struct Rating {
    int team_id;
    int elo_rating = 1500;
    double epochtime;
};

// Calculate the expected outcome probability
double calculateExpectedOutcome(int ratingA, int ratingB) {
    return 1.0 / (1.0 + std::pow(10.0, static_cast<double>(ratingB - ratingA) / 400.0));
}

// Update the Elo ratings based on the actual outcome
// outcome is 1 for win, 0 for loss, 0.5 for tie
void updateEloRatings(const Game& game, Rating& ratingA, Rating& ratingB){ //), double outcome) {
    int atr = game.away_team_runs;
    int htr = game.home_team_runs;
    int aid = game.away_team_id;
    int hid = game.home_team_id;

    if(atr > htr){
	    std::cout << aid << " beat " << hid << "," << atr << "-" << htr << std::endl;
    }else if (game.home_team_runs > game.away_team_runs){
	    std::cout << hid << " beat " << aid << "," << htr << "-" << atr << std::endl;
    }
}

int selectDataCallback(void* data, int argc, char** argv, char** /*azColName*/) {
    std::vector<Game>* games = static_cast<std::vector<Game>*>(data);

    Game game;
    game.id = std::stoi(argv[0]);
    game.home_team_id = std::stoi(argv[1]);
    game.home_team_runs = std::stoi(argv[2]);
    game.away_team_id = std::stoi(argv[3]);
    game.away_team_runs = std::stoi(argv[4]);
    game.year = std::stoi(argv[5]);
    game.epochtime = std::stod(argv[6]);
    games->push_back(game);

    return 0;
}

int main() {
    sqlite3* db;
    char* errorMsg = nullptr;

    // Open a connection to the database
    int rc = sqlite3_open("mlb_data.sqlite", &db);
    if (rc != SQLITE_OK) {
        std::cout << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return rc;
    }

    // Query the table
    const char* selectDataQuery = "SELECT id, home_team_id, home_team_runs, away_team_id, away_team_runs, year , epochtime FROM Games WHERE year=2015";
    std::vector<Game> games;
    rc = sqlite3_exec(db, selectDataQuery, selectDataCallback, &games, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cout << "SQL error: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        sqlite3_close(db);
        return rc;
    }

    // Close the database connection
    sqlite3_close(db);

    std::vector<Rating> ratings;
    for(int i=0;i<NUM_TEAMS;i++){
	Rating rating;
	rating.team_id = i;
    	rating.epochtime = games[0].epochtime;
	ratings.push_back(rating);	
        std::cout<<rating.team_id <<" "<<rating.epochtime<<std::endl;
    }

    // Display the retrieved data
    for (const auto& game : games) {
      updateEloRatings(game, ratings[game.home_team_id-1], ratings[game.away_team_id-1]); 
      }


    std::cout << "Total size of the table: " << games.size() << std::endl;



    return 0;
}
