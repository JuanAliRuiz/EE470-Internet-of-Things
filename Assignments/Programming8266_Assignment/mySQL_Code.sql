-- Sensor Register Table
CREATE TABLE SensorRegister (
  node_name     VARCHAR(10)  NOT NULL,
  manufacturer  VARCHAR(10) NOT NULL,
  timezone_id   VARCHAR(64)  NOT NULL DEFAULT 'America/Los_Angeles', -- User inputs their timezone otherwise use America/Los_Angeles by default
  PRIMARY KEY (node_name)
) ENGINE=InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_general_ci;


-- Create the Sensor Activity Table
CREATE TABLE SensorActivity (
  node_name  VARCHAR(10) NOT NULL,
  msg_count  BIGINT      NOT NULL DEFAULT 0,  -- counter for the amount of messages the node has sent
  PRIMARY KEY (node_name),
  CONSTRAINT fk_act_node
    FOREIGN KEY (node_name)  -- Links the activity back to the node
    REFERENCES SensorRegister(node_name)
    ON DELETE CASCADE
    ON UPDATE CASCADE
) ENGINE=InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_general_ci;


-- Create after SensorRegister
CREATE TABLE SensorData (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  node_name     VARCHAR(64)     NOT NULL,
  time_received DATETIME        NOT NULL,      -- MUST be provided by MCU
  temp          DOUBLE          NULL,
  humidity      DOUBLE          NULL,
  light         DOUBLE          NULL,  -- All the different types of data the Sensors can collect
  proximity     DOUBLE          NULL,
  source        ENUM('SWITCH','TILT') NULL,    -- which trigger sent it
  PRIMARY KEY (id),
  UNIQUE KEY uniq_node_time (node_name, time_received),   -- blocks duplicates
  CONSTRAINT fk_data_node
    FOREIGN KEY (node_name)
    REFERENCES SensorRegister(node_name)
    ON DELETE RESTRICT
    ON UPDATE CASCADE
) ENGINE=InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_general_ci;
  
  


DELIMITER //
CREATE TRIGGER trg_sensor_activity_bump //defines the trigger that will update the data counter
AFTER INSERT ON SensorData
FOR EACH ROW
BEGIN
  INSERT INTO SensorActivity (node_name, msg_count) -- Every time something is added into a row, add to the counter
  VALUES (NEW.node_name, 1)
  ON DUPLICATE KEY UPDATE msg_count = msg_count + 1; -- If the same load sends data again, add 1 to the counter
END//
DELIMITER ;
