Lorién Portella and Lucas García Mateu

BOUNCE EM UP
Bounce em up is a 2 v 2 cooperative multiplayer top down shooter game.

GRAPHICS
Screenshots of Legue Of Legends 3d models (Ashe, Braum, Jayce and Mafia Braum).

CONTROLS
- Move around with WSAD
- Aim with the mouse
- Shoot with left click (Only "Shooter")

MECHANICS
There are two mandatory roles to be fulfilled by each team.

**Shooter**: Shoots non lethal bullets which can bounce in the reflector's barrier.
**Reflector**: Reflects the non lethal bullets, making them able to harm the enemy team.

WIN CONDITION
If either the shooter or the reflector of a team are hit by reflected bullets 4 times, the oposing team is victorious, and the game will reset.

FEATURES
- Pings -> Lucas García
- Replication -> Lucas García and Lorién Portella
- Delivery Manager -> Lorién Portella
- Entity interpolation -> Lucas García
- Gameplay -> Lucas García
- Player Client Side prediction -> Lorién Portella
- Arrow Client Side prediction -> Lucas García
- Animation system, modifying the original DirectX renderer (only used for player lifes, but it is there) -> Lucas García



CONSTRAINTS
- The game won't start until all the players are connected.
- Connected players need to be fulfilling the remaining roles, otherwise they will be kicked from the server.
(we ran out of time to implement matchmaking :( )
	Ej: Team 1-shooter and Team 2-shooter are connected. If a client tries to connect as a shooter from either team
	    he will be kicked out.
- If a player disconnects during either a match or matchmaking, all the client proxies will be destroyed in the server,
leading to the eventual disconnection of the clients due to lack of pings


Find the source code at: https://github.com/Skyway-y-su-Lacayo/Project-Algo