#!/bin/bash

diff <(./data-structures 1 2>/dev/null | sort -n) <(./data-structures 2 2>/dev/null | sort -n )
