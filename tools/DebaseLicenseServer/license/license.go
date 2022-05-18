package license

import (
	"crypto/rand"
	"crypto/sha512"
	"fmt"
	"regexp"
	"strings"
)

// License types used by both database and HTTP

type Email string
type MachineInfo string

// LicenseCode / MachineId must be type aliases instead of real types, because they're used as keys
// in our Firestore database. Firestore (and the reflect package) doesn't implicitly convert
// them to/from strings, so we have to use a type alias instead.
// There was a bug in Go 1.18, where the reflect package does implicitly convert map keys to
// non-primitive types so we got the behavior we wanted, temporarily:
//
//     https://groups.google.com/g/golang-nuts/c/GhfT2tr4Zso
//
type LicenseCode = string
type MachineId = string

type Version = uint32

const MachineIdLen = 2 * sha512.Size256 // 2* because 1 byte == 2 hex characters
const MachineInfoMaxLen = 256
const LicenseCodeLen = 10

var LicenseCodeAlphabet = []byte("abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789") // omits: ilo01

func hashStringSanitize(str string) (string, error) {
	str = strings.ToLower(str)

	validHash := regexp.MustCompile(`^[a-f0-9]+$`)
	if !validHash.MatchString(str) {
		return "", fmt.Errorf("invalid characters")
	}

	return str, nil
}

func EmailForString(str string) (Email, error) {
	// Trim space from the email as an affordance to the user
	str = strings.TrimSpace(str)

	regex := regexp.MustCompile(`.+@.+`)
	if !regex.MatchString(str) {
		return "", fmt.Errorf("invalid email")
	}

	// Lowercase the email
	email := strings.ToLower(str)
	return Email(email), nil
}

func MachineIdForString(str string) (MachineId, error) {
	if len(str) != MachineIdLen {
		return "", fmt.Errorf("invalid length")
	}

	str, err := hashStringSanitize(str)
	return MachineId(str), err
}

func MachineInfoForString(str string) MachineInfo {
	if len(str) > MachineInfoMaxLen {
		return MachineInfo(str[0:MachineInfoMaxLen])
	}
	return MachineInfo(str)
}

// func userIdForEmail(email Email) (UserId, error) {
//     if len(uid) != UserIdLen {
//         return "", fmt.Errorf("invalid length")
//     }
//
//     r, err := hashStringSanitize(string(uid))
//     return UserId(r), err
// }

func LicenseCodeForString(str string) (LicenseCode, error) {
	// Trim space from the license code as an affordance to the user
	str = strings.TrimSpace(str)

	regex := regexp.MustCompile(`^[a-zA-Z0-9]+$`)
	if !regex.MatchString(str) {
		return "", fmt.Errorf("invalid characters")
	}
	return LicenseCode(str), nil
}

func LicenseCodeGenerate() (LicenseCode, error) {
	// Generate `LicenseCodeLen` random integers, which we'll use to index into `LicenseCodeAlphabet`
	var r [LicenseCodeLen]byte
	_, err := rand.Read(r[:])
	if err != nil {
		return "", fmt.Errorf("rand.Read failed: %w", err)
	}

	// Generate our random string by indexing into LicenseCodeAlphabet
	var x [LicenseCodeLen]byte
	for i := range x {
		x[i] = LicenseCodeAlphabet[int(r[i])%len(LicenseCodeAlphabet)]
	}

	return LicenseCode(x[:]), nil
}
