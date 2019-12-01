BOUNCE EM UP
Bounce em up is a 2 v 2 cooperative multiplayer top down shooter game. 


CONTROLS
- Move around with WSAD
- Aim with the mouse
- Shoot with left click

MECHANICS
There are two mandatory roles to be fulfilled by each team.

Shooter: Shoots non lethal bullets which can bounce in the reflector's barrier.
Reflector: Reflects the non lethal bullets, making them able to harm the enemy team.

WIN CONDITION
If either the shooter or the reflector of a team are killed, the oposing team is victorious, and the game will reset.

CONSTRAINTS
- The game won't start until all the players are connected.
- Connected players need to be fulfilling the remaining roles, otherwise they will be kicked from the server.
(we run out of time to implement matchmaking :( )
	Ej: Team 1-shooter and Team 2-shooter are connected. If a client tries to connect as a shooter from either team
	    he will be kicked out.
- If a player disconnects during either a match or matchmaking, all the client proxies will be destroyed in the server,
leading to the eventual disconnection of the clients due to lack of pings

FEATURES

- Delivery Manager
- Client Side prediction
- Entity interpolation
- Animation system, modifying the original DirectX renderer (only used for player lifes, but it is there)