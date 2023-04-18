CREATE DATABASE IF NOT EXISTS `test` DEFAULT CHARACTER SET utf8mb4;

GRANT ALL PRIVILEGES ON test.* TO 'flyzz'@'%' IDENTIFIED BY '20010121';
FLUSH PRIVILEGES;

set global max_connections = 1800;
SET GLOBAL connect_timeout=45;
SET GLOBAL net_read_timeout = 45;


CREATE TABLE `user` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) DEFAULT NULL,
  `age` tinyint(4) DEFAULT NULL,
  `create_time` datetime DEFAULT NULL,
  `update_time` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `stu` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) DEFAULT NULL,
  `age` tinyint(4) DEFAULT NULL,
  `create_time` datetime DEFAULT NULL,
  `update_time` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

delimiter //
DROP PROCEDURE IF EXISTS proc_batch_insert;
CREATE PROCEDURE proc_batch_insert()
BEGIN
DECLARE pre_name BIGINT;
DECLARE ageVal INT;
DECLARE i INT;
SET pre_name=187635267;
SET ageVal=100;
SET i=1;
WHILE i <= 1000000 DO
        INSERT INTO user(`name`,age,create_time,update_time) VALUES(CONCAT(pre_name,"zzz"),(ageVal+1)%100,NOW(),NOW());
SET pre_name=pre_name+100;
SET i=i+1;
END WHILE;
END //
 
delimiter ;
call proc_batch_insert();

FLUSH TABLES;