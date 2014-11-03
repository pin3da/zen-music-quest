Zen Music Quest - Doc
=====================


[architecture](https://github.com/pin3da/zen-music-quest/blob/master/doc/arch.md)


### Port usage

| Component 1 (bind)  | Component 2 (connect)  | Port range   |
| ------------- |---------------| -----        |
| server   | server        | 4444 - 4544  |
| server   | client        | 5555 - 5655  |
| broker   | client        | 6667         |
| broker   | servers       | 6668         |

