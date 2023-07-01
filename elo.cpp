#include <cmath>
#include <iostream>
#include <sqlite3.h>
#include <vector>

#define REBASE_FACTOR 0.85
#define K_FACTOR 1
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
double calculateExpectedOutcome(int ratingH, int ratingA) {
    return 1.5 / (1.0 + std::pow(10.0, static_cast<double>(ratingH - ratingA) / 400.0));
}

// Update the Elo ratings based on the actual outcome
// outcome is 1 for win, 0 for loss, 0.5 for tie
void updateEloRatings(const Game& game, Rating& ratingH, Rating& ratingA){ //), double outcome) {
    int atr = game.away_team_runs;
    int htr = game.home_team_runs;
    int aid = game.away_team_id;
    int hid = game.home_team_id;

    int ratingDifference = ratingH.elo_rating - ratingA.elo_rating;
    double expectedOutcome = calculateExpectedOutcome(ratingH.elo_rating, ratingA.elo_rating);
    int ratingChange = static_cast<int>(K_FACTOR * (htr - atr - expectedOutcome));

    ratingH.epochtime = game.epochtime;
    ratingA.epochtime =game.epochtime;

    ratingH.elo_rating += ratingChange;
    ratingA.elo_rating -= ratingChange;


    /*if(atr > htr){
	    std::cout << aid << " beat " << hid << "," << atr << "-" << htr << std::endl;
            std::cout << ratingA.team_id+1 << " over " << ratingH.team_id+1 << std::endl;
    }else if (game.home_team_runs > game.away_team_runs){
	    std::cout << hid << " beat " << aid << "," << htr << "-" << atr << std::endl;
            std::cout << ratingH.team_id+1 << " over " << ratingA.team_id+1 << std::endl;
    }*/
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
    const char* selectDataQuery = "SELECT id, home_team_id, home_team_runs, away_team_id, away_team_runs,"
    " year , epochtime FROM Games WHERE year >= 2015 and year <= 2017"; 
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
    std::vector<Rating> ratings_history;
    for(int i=0;i<NUM_TEAMS;i++){
	Rating rating;
	rating.team_id = i;
    	rating.epochtime = games[0].epochtime;
	ratings.push_back(rating);	
        //std::cout<<rating.team_id <<" "<<rating.epochtime<<std::endl;
    }

    // Display the retrieved data
    int processing_year = games[0].year;
    for (int i=0;i<games.size();i++) {
      if (games[i].year > processing_year){
        for (auto& rating : ratings){
            rating.elo_rating = (REBASE_FACTOR)*rating.elo_rating+(1-REBASE_FACTOR)*1500; //carry over some of past rating
        }
        processing_year = games[i].year; //change the processing year
      }
      updateEloRatings(games[i], ratings[games[i].home_team_id-1], ratings[games[i].away_team_id-1]);
      std::cout<<ratings[1].team_id+1 << " rating: " << ratings[1].elo_rating 
      << "epochtime: " << ratings[1].epochtime << " in year " << games[i].year << std::endl;
      ratings_history.push_back(ratings[games[i].home_team_id-1]);
      ratings_history.push_back(ratings[games[i].away_team_id-1]);
      
    }

    for(const auto& rating : ratings_history){
        std::cout<<rating.team_id+1 << " rating " << rating.elo_rating 
        << " at date " << rating.epochtime <<std::endl;
    }
    std::cout << "Total size of ALL ratings: " << ratings_history.size() << std::endl;

    // Open the SQLite database
    rc = sqlite3_open("mlb_data.sqlite", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // Create the table if it doesn't exist
    std::string createQuery = "CREATE TABLE IF NOT EXISTS ratings ("
                              "team_id INTEGER, "
                              "elo_rating INTEGER, "
                              "epochtime REAL);";
    rc = sqlite3_exec(db, createQuery.c_str(), nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error creating table: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        sqlite3_close(db);
        return 1;
    }

    // Prepare the INSERT statement using INSERT INTO VALUES 
    std::string insertQuery = "INSERT INTO ratings (team_id, elo_rating, epochtime) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, insertQuery.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // Bind and execute the INSERT statement for each Rating object in the ratings_history collection
    for (const auto &rating : ratings_history) {
        sqlite3_bind_int(stmt, 1, rating.team_id);
        sqlite3_bind_int(stmt, 2, rating.elo_rating);
        sqlite3_bind_double(stmt, 3, rating.epochtime);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Error executing statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return 1;
        }

        sqlite3_reset(stmt);
    }

    // Finalize the statement and close the database connection
    sqlite3_finalize(stmt);
    sqlite3_close(db);



    return 0;
}