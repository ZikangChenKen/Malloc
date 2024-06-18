/*
 * The public interface to the students' memory allocator.
 */

int	 mm_init(void);
void	*mm_malloc(size_t size);
void	 mm_free(void *ptr);
void	*mm_realloc(void *ptr, size_t size);

/*
 * Students work in teams of one or two.  Teams enter their team name, personal
 * names and login IDs in a struct of this type in their mm.c file.
 */
typedef struct {
	char	*teamname;	/* Team name. */
	char	*name1;		/* Full name of first member. */
	char	*id1;		/* NetID of first member. */
	char	*name2;		/* Full name of second member (if any). */
	char	*id2;		/* NetID of second member. */
} team_t;

extern team_t team;
