
διορθωση στον ληφθέντα κωδικά του server

Αντιστροφή / Εξήγηση γιατί ο client βλέπει "Send verification code: <code>" αλλά ο server απαντά "invalid code":

- Τερματισμός γραμμής / μορφοποίηση: ο server μπορεί να περιμένει τον κωδικό με συγκεκριμένο τερματικό χαρακτήρα (π.χ. `\n`). Αν ο client τυπώνει τον κωδικό χωρίς να στείλει την αλλαγή γραμμής, ή αν αποκόπτει/αφαιρεί χαρακτήρες, ο server μπορεί να θεωρήσει τον κωδικό λανθασμένο.
- Null-termination: αν ο buffer δεν είναι σωστά `\0`-τερματισμένος ή αν στέλνεται `sizeof(buffer)` αντί για `strlen(buffer)`, τότε στέλνονται επιπλέον σκουπίδια (garbage bytes) που κάνουν το σύγκριση του server να αποτύχει.
- CRLF vs LF: διαφορές `\r\n` vs `\n` (Windows vs Unix) μπορεί να προκαλέσουν mismatch.
- Περιττοί χαρακτήρες / whitespace: προηγούμενοι χαρακτήρες στο buffer (αν δεν καθαρίζεται) ή trailing spaces θα αλλάξουν το byte-sequence.
- Μερικά writes μπορεί να είναι μερικώς αποστέλλονται (partial write) ή ο server διαβάζει πριν ολοκληρωθεί το write — βεβαιωθείτε ότι προσέχετε την τιμή επιστροφής της `write()` και επαναλαμβάνετε αν χρειάζεται.
- Χρονισμός / Πρωτόκολλο: ο server μπορεί να αναμένει να λάβει τον κωδικό σε νέα γραμμή ή μετά από συγκεκριμένο prompt. Αν στέλνετε αμέσως πίσω χωρίς newline ή με λάθος framing, το server μπορεί να μην το αντιληφθεί σωστά.
- Κωδικοποίηση / μη ορατοί χαρακτήρες: μη-εκτυπώσιμοι χαρακτήρες ή UTF-8 vs ASCII προβλήματα μπορούν να αλλάξουν το μήνυμα.

Συνιστώμενοι έλεγχοι (βήματα debugging):

- Εκτυπώστε τα bytes που στέλνετε σε hex πριν το `write()` (`for (i=0;i<strlen(buf);i++) printf("%02x ", (unsigned char)buf[i]);`) και συγκρίνετε με τα bytes που λαμβάνει ο server (tcpdump ή server log).
- Σιγουρευτείτε ότι στέλνετε την αλλαγή γραμμής αν ο server την περιμένει: `snprintf(tmp, sizeof(tmp), "%s\n", verification_buffer); write(sockfd, tmp, strlen(tmp));`
- Μην χρησιμοποιείτε `write(sockfd, buffer, sizeof(buffer))` — χρησιμοποιήστε `strlen(buffer)` ή την ακριβή byte count από `snprintf`/`sprintf`.
- Ελέγξτε την τιμή επιστροφής της `write()` και επαναλάβετε αν χρειάζεται μέχρι να στείλετε όλα τα bytes.
- Καθαρίζετε το buffer πριν τη χρήση (`memset(verification_buffer,0,sizeof verification_buffer);`) και αποφύγετε να στέλνετε παλιά δεδομένα.
- Δοκιμάστε προσωρινά να στείλετε πίσω τον ακριβή κωδικό συν ένα `\n` χειροκίνητα (copy-paste) για να επιβεβαιώσετε ότι ο server αποδέχεται τη σωστή ακολουθία.
- Χρησιμοποιήστε `tcpdump -A -s 0 port 4217` ή `ngrep` για να δείτε τι ακριβώς μεταφέρεται στο δίκτυο.

Παράδειγμα ασφαλούς send (C):

```
char out[BUFFER_SIZE];
snprintf(out, sizeof(out), "%s\n", verification_buffer);
size_t to_write = strlen(out);
size_t written = 0;
while (written < to_write) {
	ssize_t w = write(sockfd, out + written, to_write - written);
	if (w <= 0) { perror("write"); break; }
	written += w;
}
```

Χρησιμοποιήστε αυτό το PR για να συζητήσουμε και να προσαρμόσουμε τον client ώστε να στέλνει ακριβώς τη μορφή που περιμένει ο server.

