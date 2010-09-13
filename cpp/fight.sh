#!/bin/bash
java -jar tools/PlayGame-1.2.jar maps/map17.txt 1000 1000 log.txt "java -jar example_bots/RageBot.jar" "./MyBot" | java -jar tools/ShowGame-1.2.jar
