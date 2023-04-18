package main

import (
	"database/sql"
	"fmt"
	"log"
	"math/rand"
	"time"

	"github.com/gin-gonic/gin"
	_ "github.com/go-sql-driver/mysql"
)

type User struct {
    ID   int    `json:"id"`
    Name string `json:"name"`
	Age  int    `json:"age"`
	Create_time time.Time `json:"create_time"`
	Update_time time.Time `json:"update_time"`
}

func main() {
	rand.Seed(time.Now().UnixNano())
	gin.SetMode(gin.ReleaseMode)
	gin.DisableConsoleColor()
    r := gin.Default()

    db, err := sql.Open("mysql", "flyzz:20010121@tcp(192.168.125.81:3306)/test?parseTime=true")
    if err != nil {
        log.Fatal(err)
    }
    defer db.Close()

	if err = db.Ping(); err != nil {
		log.Fatal(err)
	}

	db.SetMaxOpenConns(1800)
	db.SetMaxIdleConns(700)
	//stu_n := 100000
	user_n := 1000000
    r.GET("/select", func(c *gin.Context) {

		stmt, err := db.Prepare("SELECT * FROM user WHERE id = ?")
		if err != nil {
			log.Fatal(err)
		}
		defer stmt.Close()

		row := stmt.QueryRow(rand.Intn(user_n))
		var user User
		err = row.Scan(&user.ID, &user.Name, &user.Age, &user.Create_time, &user.Update_time)


        if err != nil {
			fmt.Println(err)
			c.AbortWithError(404,fmt.Errorf("select error"))
			return
        }

        c.JSON(200, gin.H{
            "user": user,
        })
    })

	r.GET("/selectall", func(c *gin.Context) {
		stmt, err := db.Prepare("SELECT * FROM user WHERE id > ? LIMIT 100")
		if err != nil {
			log.Fatal(err)
		}
		defer stmt.Close()
		rows,err := stmt.Query(rand.Intn(user_n))
		if err != nil {
			fmt.Println(err)
			c.AbortWithError(404,fmt.Errorf("selectall error"))
			return
		}
		var stus []User
		for rows.Next() {
			var user User
			err = rows.Scan(&user.ID, &user.Name, &user.Age, &user.Create_time, &user.Update_time)
			if err != nil {
				fmt.Println(err)
				c.AbortWithError(404,fmt.Errorf("selectall error"))
				return
			}
			stus = append(stus, user)
		}

		c.JSON(200, gin.H{
			"users": stus,
		})
	})


	r.GET("/insert", func(c *gin.Context) {
		stmt, err := db.Prepare("INSERT INTO user(name, age, create_time, update_time) VALUES(?, ?, ?, ?)")
		if err != nil {
			log.Fatal(err)
		}
		defer stmt.Close()

		now := time.Now()
		_, err = stmt.Exec("zzz", rand.Intn(100), now, now)
		if err != nil {
			c.AbortWithError(404,fmt.Errorf("insert error"))
			return
		}

		c.JSON(200, gin.H{
			"message": "insert success",
		})
	})

	r.GET("/update", func(c *gin.Context) {
		stmt, err := db.Prepare("UPDATE user SET name = ?, age = ? WHERE id = ?")
		if err != nil {
			log.Fatal(err)
		}
		defer stmt.Close()

		_, err = stmt.Exec("zzz", rand.Intn(100), rand.Intn(user_n))
		if err != nil {
			c.AbortWithError(404,fmt.Errorf("update error"))
			return
		}

		c.JSON(200, gin.H{
			"message": "update success",
		})
	})

    if err := r.Run(":8080"); err != nil {
        log.Fatal(err)
    }
}