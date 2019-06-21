class Algorithms {


/*

algorithm quicksort(A, lo, hi) is
    if lo < hi then
        p := partition(A, lo, hi)
        quicksort(A, lo, p - 1)
        quicksort(A, p + 1, hi)

algorithm partition(A, lo, hi) is
    pivot := A[hi]
    i := lo
    for j := lo to hi - 1 do
        if A[j] < pivot then
            swap A[i] with A[j]
            i := i + 1
    swap A[i] with A[hi]
    return i
*/
static swap_(A, i, j) {
    var tmp = A[i]
    A[i] = A[j]
    A[j] = tmp
}

static partition_(A, lo, hi) {
    var pivot = A[hi]
    var i = lo
    for (j in lo ... hi) {
        if (A[j] < pivot) {
            swap_(A, i, j)
            i = i + 1
        }
    }
    swap_(A, i, hi)
    return i
}

static qsort(A, lo, hi) {
    if (lo < hi) {
        var p = partition_(A, lo, hi)
        qsort(A, lo, p - 1)
        qsort(A, p + 1, hi)
    }
}

// Merges two subarrays of arr[].
// First subarray is arr[l..m]
// Second subarray is arr[m+1..r]
// Inplace Implmentation
static merge_(arr, start, mid, end) {
    var start2 = mid + 1
    // If the direct merge is already sorted
    if (arr[mid] <= arr[start2]) {
        return
    }
    // Two pointers to maintain start
    // of both arrays to merge
    while (start <= mid && start2 <= end) {
        // If element 1 is in right place
        if (arr[start] <= arr[start2]) {
            start = start + 1
        } else {
            var value = arr[start2]
            var index = start2
            // Shift all the elements between element 1
            // element 2, right by 1.
            while (index != start) {
                arr[index] = arr[index - 1]
                index = index - 1
            }
            arr[start] = value
            // Update all the pointers
            start = start + 1
            mid = mid + 1
            start2 = start2 + 1
        }
    }
}


/* l is for left index and r is right index of the
   sub-array of arr to be sorted */
static msort(arr, l, r) {
    if (l < r) {
        // Same as (l + r) / 2, but avoids overflow
        // for large l and r
        var m = l + ((r - l) / 2).floor
        // Sort first and second halves
        msort(arr, l, m)
        msort(arr, m + 1, r)
        merge_(arr, l, m, r)
    }
}

    static isort(L, f) {
        // Optimized insertion-sort.
        if (L.count == 0) {
            return
        }
        for (i in 1 ... L.count) {
            var x = L[i]
            var j = i - 1
            while (j >= 0 && f.call(L[j], x) > 0) {
                L[j + 1] = L[j]
                j = j - 1
            }
            L[j + 1] = x
        }
    }

    static sort(L, f) {
        isort(L, f)
    }

    static sorted(L, f) {
        for (i in 1 ... L.count) {
            if (f.call(L[i - 1], L[i]) > 0) {
                return false
            }
        }
        return true
    }

}

