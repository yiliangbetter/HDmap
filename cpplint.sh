#!/bin/bash
find . -type f \( -name "*.cpp" -o -name "*.h" \) -exec cpplint --filter=-legal/copyright,-build/include_order {} +