USE Pokemon;

SELECT name FROM Trainer WHERE hometown = 'Blue City';

SELECT name FROM Trainer WHERE hometown = 'Brown City' OR hometown = 'Rainbow City';

SELECT name, hometown FROM Trainer WHERE name LIKE 'a%' OR name LIKE 'e%' OR name LIKE 'i%' OR name LIKE 'o%' OR name LIKE 'u%';

SELECT name FROM Pokemon WHERE type = 'Water';

SELECT DISTINCT type FROM Pokemon;

SELECT name FROM Pokemon ORDER BY name ASC;

SELECT name FROM Pokemon WHERE name LIKE '%s';

SELECT name FROM Pokemon WHERE name LIKE '%e%s';

SELECT name FROM Pokemon WHERE name LIKE 'a%' OR name LIKE 'e%' OR name LIKE 'i%' OR name LIKE 'o%' OR name LIKE 'u%';

SELECT type, COUNT(*) AS '#types' FROM Pokemon GROUP BY type;

SELECT nickname FROM CatchedPokemon ORDER BY level DESC LIMIT 3;

SELECT AVG(level) AS Avg_level FROM CatchedPokemon;

SELECT MAX(level)-MIN(level) AS Difference FROM CatchedPokemon;

SELECT COUNT(*) AS '#pokemon' FROM Pokemon WHERE name >= 'b%' AND name < 'f%';

SELECT COUNT(*) AS '#pokemon' FROM Pokemon WHERE type NOT IN ('Fire', 'Grass', 'Water', 'Electric');

SELECT Trainer.name, Pokemon.name, CatchedPokemon.nickname FROM CatchedPokemon JOIN Pokemon ON CatchedPokemon.pid = Pokemon.id JOIN Trainer ON CatchedPokemon.owner_id = Trainer.id WHERE nickname LIKE '% %';

SELECT Trainer.name FROM CatchedPokemon, Pokemon, Trainer WHERE CatchedPokemon.pid = Pokemon.id AND CatchedPokemon.owner_id = Trainer.id AND Pokemon.type = 'Psychic';

SELECT Trainer.name, Trainer.hometown FROM CatchedPokemon, Trainer WHERE CatchedPokemon.owner_id = Trainer.id GROUP BY Trainer.name ORDER BY AVG(CatchedPokemon.level) DESC LIMIT 3;

SELECT Trainer.name, COUNT(*) AS '#catched' FROM CatchedPokemon, Trainer WHERE CatchedPokemon.owner_id = Trainer.id GROUP BY Trainer.name ORDER BY COUNT(*) DESC, Trainer.name DESC;

SELECT Pokemon.name, CatchedPokemon.level FROM Gym, Trainer, Pokemon, CatchedPokemon WHERE Gym.leader_id = Trainer.id AND CatchedPokemon.owner_id = Trainer.id AND CatchedPokemon.pid = Pokemon.id AND Gym.city = 'Sangnok City' ORDER BY level ASC;

SELECT Pokemon.name, IF(ISNULL(CatchedPokemon.pid), 0, COUNT(*)) AS '#catched' FROM Pokemon LEFT JOIN CatchedPokemon ON Pokemon.id = CatchedPokemon.pid GROUP BY Pokemon.name ORDER BY IF(ISNULL(CatchedPokemon.pid), 0, COUNT(*)) DESC;

SELECT C.name FROM Pokemon AS A, Pokemon AS B, Pokemon AS C, Evolution AS E1, Evolution AS E2 WHERE E1.before_id = A.id AND E1.After_id = B.id AND E2.before_id = B.id AND E2.after_id = C.id AND A.name = 'Charmander';

SELECT Pokemon.name FROM Pokemon, CatchedPokemon WHERE Pokemon.id = CatchedPokemon.pid AND Pokemon.id <= 30 GROUP BY Pokemon.name ORDER BY Pokemon.name ASC;

SELECT Trainer.name, Pokemon.type FROM Trainer, Pokemon, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id AND Pokemon.id = CatchedPokemon.pid GROUP BY Trainer.name HAVING COUNT(DISTINCT Pokemon.type) = 1;

SELECT Trainer.name, Pokemon.type, COUNT(*) AS '#types' FROM Trainer, Pokemon, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id AND Pokemon.id = CatchedPokemon.pid GROUP BY Trainer.name, Pokemon.type;


SELECT Trainer.name, Pokemon.name, COUNT(*) AS '#catched' FROM Trainer, Pokemon, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id AND Pokemon.id = CatchedPokemon.pid GROUP BY Trainer.name HAVING COUNT(DISTINCT Pokemon.name) = 1;

SELECT Trainer.name, Gym.city FROM Trainer, Gym WHERE Trainer.id = Gym.leader_id AND Trainer.name IN (SELECT Trainer.name FROM Trainer, Pokemon, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id AND Pokemon.id = CatchedPokemon.pid GROUP BY Trainer.name HAVING NOT COUNT(DISTINCT Pokemon.type) = 1);

SELECT Trainer.name, SUM(IF(CatchedPokemon.level < 50, NULL, level)) AS 'sum_level' FROM Trainer, Gym, CatchedPokemon WHERE Trainer.id = Gym.leader_id AND CatchedPokemon.owner_id = Trainer.id GROUP BY Trainer.name;

SELECT Pokemon.name FROM Trainer, Pokemon, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id AND Pokemon.id = CatchedPokemon.pid AND Trainer.hometown IN ('Sangnok City', 'Blue City') GROUP BY Pokemon.name HAVING COUNT(DISTINCT(Trainer.hometown)) = 2;

SELECT Pokemon.name FROM Pokemon WHERE Pokemon.id IN (SELECT after_id FROM Evolution) AND Pokemon.id NOT IN (SELECT before_id FROM Evolution);
