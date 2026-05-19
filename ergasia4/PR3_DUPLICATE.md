Duplicate of PR #2

This PR duplicates the merged PR #2 content for follow-up discussion.

Original PR comment (copied):

"Πρέπει να λυθεί το εξής: 'Send verification code: 38c34793bcdfcc4813' -> 'Server: invalid code'. Έλεγξε αν στέλνεις τον χαρακτήρα τέλους (\0); στέλνεις έξτρα αλλαγή γραμμής (\n); καθαρίζεις τα buffers σου; Μήπως στέλνεις sizeof(buffer) αντί για τα πραγματικά bytes;"

Use this PR to continue discussion or attach additional fixes.
