/* SHOW DATABASES; */
    /* USE u148378080_juanaliruiz; */
    /*   
CREATE TABLE sensor_data (
    node_name VARCHAR(10) NOT NULL PRIMARY KEY,
    time_received DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    temperature DECIMAL(6,2) NOT NULL CHECK (temperature BETWEEN -10 AND 100),
    humidity DECIMAL(6,2) NOT NULL CHECK (humidity BETWEEN 0 AND 100));
    


CREATE TABLE sensor_register (
    node_name VARCHAR(10) NOT NULL PRIMARY KEY,
    manufacturer VARCHAR(10) NOT NULL,
    longitude DECIMAL(15,8) NOT NULL,
    latitude DECIMAL(15,8) NOT NULL,
    FOREIGN KEY (node_name)
    	REFERENCES sensor_register (node_name)
    	ON DELETE CASCADE
    	ON UPDATE CASCADE);


INSERT INTO sensor_register (node_name, manufacturer, longitude, latitude)
VALUES
('node_1', 'Acme', -122.41940000, 37.77490000),
('node_2', 'Beta', -118.24370000, 34.05220000),
('node_3', 'Alpha', -117.16110000, 32.71570000),
('node_4', 'Echo', -121.88630000, 37.33820000),
('node_5', 'Delta', -119.41790000, 36.77830000);


ALTER TABLE sensor_register
  DROP FOREIGN KEY sensor_register_ibfk_1;


SELECT DISTINCT d.node_name
FROM sensor_data d
LEFT JOIN sensor_register r ON r.node_name = d.node_name
WHERE r.node_name IS NULL;


ALTER TABLE sensor_data
  ADD CONSTRAINT fk_sensor
  FOREIGN KEY (node_name)
  REFERENCES sensor_register (node_name)
  ON DELETE RESTRICT
  ON UPDATE CASCADE;


INSERT INTO sensor_register (node_name, manufacturer, longitude, latitude)
VALUES
('node_1', 'Acme', -122.41940000, 37.77490000),
('node_2', 'Beta', -118.24370000, 34.05220000),
('node_3', 'Alpha', -117.16110000, 32.71570000),
('node_4', 'Echo', -121.88630000, 37.33820000),
('node_5', 'Delta', -119.41790000, 36.77830000);



ALTER TABLE sensor_data
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (node_name, time_received);
  


INSERT INTO sensor_data (node_name, time_received, temperature, humidity)
SELECT r.node_name, t.time_received,
       CAST(22.50 + t.n AS DECIMAL(6,2)),
       CAST(45.00 + 2*t.n AS DECIMAL(6,2))
FROM sensor_register r
CROSS JOIN (
  SELECT 0 AS n, TIMESTAMP('2022-10-01 11:00:00') AS time_received
  UNION ALL SELECT 1, TIMESTAMP('2022-10-01 11:30:00')
  UNION ALL SELECT 2, TIMESTAMP('2022-10-01 12:00:00')
  UNION ALL SELECT 3, TIMESTAMP('2022-10-01 12:30:00')
) AS t
ORDER BY r.node_name, t.time_received;



CREATE VIEW sensor_combined AS
SELECT
  d.node_name,
  d.time_received,
  d.temperature,
  d.humidity,
  r.manufacturer,
  r.longitude,
  r.latitude
FROM sensor_data d
JOIN sensor_register r USING (node_name);


SELECT * FROM sensor_combined ORDER BY node_name, time_received;
*/
INSERT INTO sensor_data(
    node_name,
    time_received,
    temperature,
    humidity
)
VALUES(
    'node_unreg',
    '2022-10-01 11:00:00',
    25.00,
    50.00
);
