# Alive FOBs
It scans last 200,000~ players and checks if they are online on Steam and own an FOB.

App hosts a HTTP server that returns a JSON of 10 random players from local database

## Requirements
- Steam API key
- OpenSSL
- SQLite3

## Usage
Create [.env](https://github.com/WujekFoliarz/alive-fobs/blob/main/res/.env.example) next to the executable and put your Steam API key there

You can create the .env file in [res](https://github.com/WujekFoliarz/alive-fobs/tree/main/res) folder and it will automatically copy to out folder on build.

./alive-fobs <ip ex. 0.0.0.0> <port ex. 8080>

The app will automatically start scanning and saving found players (konami id, steam id, date) to database/active_players.db

## Credits
[alicealys](https://github.com/alicealys) - Complete player database and some sqlpp macros
