# Overview

A personal project to create a classical chess AI that utilizes:
 - Minimax tree search with alpha-beta pruning
 - Basic board evaluator to be used at tree leaves

# What is the Purpose

The purpose of this project was to get some practice wiriting code in the following ways:
 - Flat, non-pessimised, data-oriented design, where the computation sits close to the logic
 - Assert based assumptions, where restrictions on other code are defined and tested with assert statements

This is all to be contrast with more contemporary approaches, namely object oriented design, that I was taught to code through during my education

# Future Plans

I plan on continuing work on this project to increase the practice I have in this coding style, adding the following features:
 - Transposition table, to cache already seen before positions
 - Iterative deepending, to allow someone to specify a time to search, compared to the depth it asks for now
 - Use previous search tree results to optimize deeper searches
 - Parallelize parts of the code, utilizing both multi-threading and SIMD instructions
